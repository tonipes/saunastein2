// Copyright (c) 2025 Inan Evin

#include "world_renderer.hpp"
#include "common/system_info.hpp"
#include "memory/memory_tracer.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/texture_queue.hpp"
#include "gfx/buffer_queue.hpp"
#include "world/world.hpp"
#include "world/traits/trait_light.hpp"
#include "world/traits/trait_model_instance.hpp"
#include "resources/mesh.hpp"
#include "resources/primitive.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include <algorithm>
#include <execution>

namespace SFG
{
#define RT_FORMAT			 format::r8g8b8a8_srgb
#define DEPTH_FORMAT		 format::d16_unorm
#define SHARED_BUMP_ALLOC_SZ 1024 * 512

	world_renderer::world_renderer(proxy_manager& pm, world& w) : _proxy_manager(pm), _world(w){};
	void world_renderer::init(const vector2ui16& size, texture_queue* tq, buffer_queue* bq)
	{
		_texture_queue = tq;
		_buffer_queue  = bq;

		gfx_backend* backend = gfx_backend::get();

		const bitmask16 color_flags = texture_flags::tf_is_2d | texture_flags::tf_render_target | texture_flags::tf_sampled;

		_base_size = size;

		static_vector<gfx_id, FRAMES_IN_FLIGHT> entity_buffers;
		static_vector<gfx_id, FRAMES_IN_FLIGHT> bone_buffers;
		static_vector<gfx_id, FRAMES_IN_FLIGHT> light_buffers;

		// pfd
		for (uint8 i = 0; i < FRAMES_IN_FLIGHT; i++)
		{
			per_frame_data& pfd	  = _pfd[i];
			pfd.sem_gfx.semaphore = backend->create_semaphore();

			pfd.bones.create_staging_hw(
				{
					.size		= sizeof(gpu_bone) * MAX_GPU_BONES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "bones_cpu",
				},
				{
					.size		= sizeof(gpu_bone) * MAX_GPU_BONES,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name = "bones_gpu",
				});

			pfd.entities.create_staging_hw(
				{
					.size		= sizeof(gpu_entity) * MAX_GPU_ENTITIES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "entities_cpu",
				},
				{
					.size		= sizeof(gpu_entity) * MAX_GPU_ENTITIES,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name = "entities_gpu",
				});

			pfd.lights.create_staging_hw(
				{
					.size		= sizeof(gpu_light) * MAX_GPU_LIGHTS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "lights_cpu",
				},
				{
					.size		= sizeof(gpu_light) * MAX_GPU_LIGHTS,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name = "lights_gpu",
				});

			entity_buffers.push_back(pfd.entities.get_hw_gpu());
			bone_buffers.push_back(pfd.bones.get_hw_gpu());
			light_buffers.push_back(pfd.lights.get_hw_gpu());
		}

		// Command allocations
		{
			const size_t total_shared_size = SHARED_BUMP_ALLOC_SZ;
			const size_t lane_count		   = 2;
			const size_t size_per_lane	   = total_shared_size / lane_count;
			_shared_command_alloc		   = (uint8*)SFG_MALLOC(total_shared_size);
			PUSH_ALLOCATION_SZ(total_shared_size);

			uint8* alloc_head = _shared_command_alloc;
			_pass_opaque.init({
				.size			= size,
				.alloc			= alloc_head,
				.alloc_size		= size_per_lane,
				.entity_buffers = entity_buffers.data(),
				.bone_buffers	= bone_buffers.data(),
			});

			alloc_head += size_per_lane;

			static_vector<gfx_id, FRAMES_IN_FLIGHT * 4> opaque_textures;
			static_vector<gfx_id, FRAMES_IN_FLIGHT>		depth_textures;

			for (uint32 i = 0; i < FRAMES_IN_FLIGHT; i++)
			{
				opaque_textures.push_back(_pass_opaque.get_color_texture(i, 0));
				opaque_textures.push_back(_pass_opaque.get_color_texture(i, 1));
				opaque_textures.push_back(_pass_opaque.get_color_texture(i, 2));
				opaque_textures.push_back(_pass_opaque.get_color_texture(i, 3));
				depth_textures.push_back(_pass_opaque.get_depth_texture(i));
			}

			//_pass_lighting_fw.init({
			//	.size			 = size,
			//	.alloc			 = alloc_head,
			//	.alloc_size		 = size_per_lane,
			//	.entity_buffers	 = entity_buffers.data(),
			//	.bone_buffers	 = bone_buffers.data(),
			//	.light_buffers	 = light_buffers.data(),
			//	.opaque_textures = opaque_textures.data(),
			//	.depth_textures	 = depth_textures.data(),
			//});
			// alloc_head += size_per_lane;
		}
	}

	void world_renderer::uninit()
	{
		_pass_opaque.uninit();
		// _pass_lighting_fw.uninit();
		// _pass_post_combiner.uninit();

		PUSH_DEALLOCATION_SZ(SHARED_BUMP_ALLOC_SZ);
		SFG_FREE(_shared_command_alloc);

		gfx_backend* backend = gfx_backend::get();

		world_render_data& rd = _render_data;

		for (uint8 i = 0; i < FRAMES_IN_FLIGHT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_semaphore(pfd.sem_gfx.semaphore);
			pfd.bones.destroy();
			pfd.entities.destroy();
			pfd.lights.destroy();
		}
	}

	void world_renderer::upload(uint8 frame_index)
	{
		world_render_data& rd = _render_data;
		rd.reset();

		// All renderables, required entities etc.
		generate_renderables();

		per_frame_data& pfd = _pfd[frame_index];

		if (!rd.bones.empty())
			pfd.bones.buffer_data(0, rd.bones.data(), sizeof(gpu_bone) * rd.bones.size());

		if (!rd.entities.empty())
			pfd.entities.buffer_data(0, rd.entities.data(), sizeof(gpu_entity) * rd.entities.size());

		if (!rd.lights.empty())
			pfd.lights.buffer_data(0, rd.lights.data(), sizeof(gpu_light) * rd.lights.size());

		_buffer_queue->add_request({.buffer = &pfd.entities});
		_buffer_queue->add_request({.buffer = &pfd.bones});
		_buffer_queue->add_request({.buffer = &pfd.lights});

		_pass_opaque.upload(_world, _buffer_queue, frame_index);

		frustum_cull();
	}

	void world_renderer::render(uint8 frame_index, gfx_id layout_global, gfx_id bind_group_global, uint64 prev_copy, uint64 next_copy, gfx_id sem_copy)
	{
		gfx_backend* backend   = gfx_backend::get();
		const gfx_id queue_gfx = backend->get_queue_gfx();

		per_frame_data&	   pfd		  = _pfd[frame_index];
		world_render_data& rd		  = _render_data;
		const vector2ui16  resolution = _base_size;

		semaphore_data& sd_opaque			= _pass_opaque.get_semaphore(frame_index);
		semaphore_data& sd_lighting_fw		= _pass_lighting_fw.get_semaphore(frame_index);
		semaphore_data& sd_post				= _pass_post_combiner.get_semaphore(frame_index);
		const gfx_id	sem_opaque			= sd_opaque.semaphore;
		const gfx_id	sem_lighting_fw		= sd_lighting_fw.semaphore;
		const gfx_id	sem_post			= sd_post.semaphore;
		const uint64	sem_opaque_val		= ++sd_opaque.value;
		const uint64	sem_lighting_fw_val = ++sd_lighting_fw.value;
		const uint64	sem_post_val		= ++sd_post.value;
		const gfx_id	cmd_opaque			= _pass_opaque.get_cmd_buffer(frame_index);
		const gfx_id	cmd_lighting_fw		= _pass_lighting_fw.get_cmd_buffer(frame_index);
		const gfx_id	cmd_post			= _pass_post_combiner.get_cmd_buffer(frame_index);

		static_vector<std::function<void()>, 1> tasks;
		tasks.push_back([&] { _pass_opaque.render(frame_index, resolution, layout_global, bind_group_global); });
		std::for_each(std::execution::par, tasks.begin(), tasks.end(), [](std::function<void()>& task) { task(); });

		//	tasks.push_back([&] { _pass_lighting_fw.render(data_index, frame_index, resolution, layout_global, bind_group_global); });
		//	tasks.push_back([&] { _pass_post_combiner.render(data_index, frame_index, resolution, layout_global, bind_group_global); });

		// Submit opaque, signals itself
		backend->submit_commands(queue_gfx, &cmd_opaque, 1);

		if (prev_copy != next_copy)
			backend->queue_wait(queue_gfx, &sem_copy, &next_copy, 1);

		backend->queue_signal(queue_gfx, &sem_opaque, &sem_opaque_val, 1);

		// Submit lighting + forward, waits on opaque, signals itself
		// backend->queue_wait(queue_gfx, &sem_opaque, &sem_opaque_val, 1);
		// backend->submit_commands(queue_gfx, &cmd_lighting_fw, 1);
		// backend->queue_signal(queue_gfx, &sem_lighting_fw, &sem_lighting_fw_val, 1);
		//
		// // Submit post combiner, waits on lighting + forward, signals itself
		// backend->queue_wait(queue_gfx, &sem_lighting_fw, &sem_lighting_fw_val, 1);
		// backend->submit_commands(queue_gfx, &cmd_post, 1);
		// backend->queue_signal(queue_gfx, &sem_post, &sem_post_val, 1);
	}

	void world_renderer::resize(const vector2ui16& size)
	{
		_base_size			 = size;
		gfx_backend* backend = gfx_backend::get();

		_pass_opaque.resize(size);
		// _pass_lighting_fw.resize(size);
		// _pass_post_combiner.resize(size);

		static_vector<gfx_id, FRAMES_IN_FLIGHT * 4> opaque_textures;
		static_vector<gfx_id, FRAMES_IN_FLIGHT>		depth_textures;

		for (uint32 i = 0; i < FRAMES_IN_FLIGHT; i++)
		{
			opaque_textures.push_back(_pass_opaque.get_color_texture(i, 0));
			opaque_textures.push_back(_pass_opaque.get_color_texture(i, 1));
			opaque_textures.push_back(_pass_opaque.get_color_texture(i, 2));
			opaque_textures.push_back(_pass_opaque.get_color_texture(i, 3));
			depth_textures.push_back(_pass_opaque.get_depth_texture(i));
		}

		// _pass_lighting_fw.reset_target_textures(opaque_textures.data(), depth_textures.data());
	}

	uint32 world_renderer::create_gpu_entity(const gpu_entity& e)
	{
		world_render_data& rd  = _render_data;
		const uint32	   idx = static_cast<uint32>(rd.entities.size());
		rd.entities.push_back(e);
		return idx;
	}

	uint16 world_renderer::create_gpu_object(const renderable_object& e)
	{
		world_render_data& rd = _render_data;
		const uint16	   i  = static_cast<uint16>(rd.objects.size());
		rd.objects.push_back(e);
		return i;
	}

	void world_renderer::push_barrier_ps(gfx_id id, static_vector<barrier, MAX_BARRIERS>& barriers)
	{
		barriers.push_back({
			.resource	= id,
			.flags		= barrier_flags::baf_is_texture,
			.from_state = resource_state::render_target,
			.to_state	= resource_state::ps_resource,
		});
	}

	void world_renderer::push_barrier_rt(gfx_id id, static_vector<barrier, MAX_BARRIERS>& barriers)
	{
		barriers.push_back({
			.resource	= id,
			.flags		= barrier_flags::baf_is_texture,
			.from_state = resource_state::ps_resource,
			.to_state	= resource_state::render_target,
		});
	}

	void world_renderer::send_barriers(gfx_id cmd_list, static_vector<barrier, MAX_BARRIERS>& barriers)
	{
		gfx_backend* backend = gfx_backend::get();
		backend->cmd_barrier(cmd_list,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<uint16>(barriers.size()),
							 });

		barriers.clear();
	}

	void world_renderer::generate_renderables()
	{
		auto&			   model_instances = _proxy_manager.get_model_instances();
		auto&			   entities		   = _proxy_manager.get_entities();
		chunk_allocator32& aux			   = _proxy_manager.get_aux();

		_proxy_manager.get_material(0);

		for (const render_proxy_model_instance& p : model_instances)
		{
			if (p.status != render_proxy_status::rps_active)
				continue;

			const render_proxy_entity& proxy_entity = entities.get(p.entity);
			SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);

			if (proxy_entity.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
				continue;

			const uint32 entity_index = create_gpu_entity({.model = proxy_entity.model});

			const render_proxy_mesh& m			= _proxy_manager.get_mesh(0);
			render_proxy_primitive*	 primitives = aux.get<render_proxy_primitive>(m.primitives);

			uint16* materials = aux.get<uint16>(p.materials);

			for (uint32 i = 0; i < m.primitive_count; i++)
			{
				const render_proxy_primitive& prim = primitives[i];

				create_gpu_object({
					.vertex_buffer = const_cast<buffer*>(&m.vertex_buffer),
					.index_buffer  = const_cast<buffer*>(&m.index_buffer),
					.vertex_start  = prim.vertex_start,
					.index_start   = prim.index_start,
					.index_count   = prim.index_count,
					.gpu_entity	   = entity_index,
					.material	   = materials[prim.material_index],
				});
			}
		}
	}

	void world_renderer::frustum_cull()
	{
		world_render_data& rd = _render_data;

		_view_manager.reset();

		//_view_manager.generate_view_from_camera(_world.get_camera());
	}
}
