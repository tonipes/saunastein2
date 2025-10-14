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
#include "world/traits/trait_mesh_instance.hpp"
#include "resources/mesh.hpp"
#include "resources/primitive.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include <algorithm>

/* DEBUG */
#include "serialization/serialization.hpp"

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

		// pfd
		for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd	  = _pfd[i];
			pfd.sem_gfx.semaphore = backend->create_semaphore();
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
				.size		= size,
				.alloc		= alloc_head,
				.alloc_size = size_per_lane,
			});

			alloc_head += size_per_lane;

			static_vector<gfx_id, BACK_BUFFER_COUNT * 4> opaque_textures;
			static_vector<gfx_id, BACK_BUFFER_COUNT>	 depth_textures;

			for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
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

		for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_semaphore(pfd.sem_gfx.semaphore);
		}
	}

	void world_renderer::prepare(uint8 frame_index)
	{
		world_render_data& rd = _render_data;
		rd.reset();

		const world_id main_cam_trait = _proxy_manager.get_main_camera();
		if (main_cam_trait == NULL_WORLD_ID)
			return;

		const render_proxy_camera& cam_proxy  = _proxy_manager.get_camera(main_cam_trait);
		const render_proxy_entity& cam_entity = _proxy_manager.get_entity(cam_proxy.entity);
		const matrix4x3&		   cam_abs	  = cam_entity.model;
		_main_camera_view					  = {
								.view_matrix = camera::view(cam_entity.rotation, cam_entity.position),
								.proj_matrix = camera::proj(cam_proxy.fov_degrees, _base_size, cam_proxy.near_plane, cam_proxy.far_plane),
		};
		_main_camera_view.view_proj_matrix = _main_camera_view.proj_matrix * _main_camera_view.view_matrix;
		_main_camera_view.view_frustum	   = frustum::extract(_main_camera_view.view_proj_matrix);

		collect_model_instances();

		_pass_opaque.prepare(_proxy_manager, _main_camera_view, frame_index, rd);
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

		// Single task; avoid parallel overhead.
		_pass_opaque.render(frame_index, rd, resolution, layout_global, bind_group_global);

		//	tasks.push_back([&] { _pass_lighting_fw.render(data_index, frame_index, resolution, layout_global, bind_group_global); });
		//	tasks.push_back([&] { _pass_post_combiner.render(data_index, frame_index, resolution, layout_global, bind_group_global); });

		if (prev_copy != next_copy)
			backend->queue_wait(queue_gfx, &sem_copy, &next_copy, 1);

		// Submit opaque, signals itself
		backend->submit_commands(queue_gfx, &cmd_opaque, 1);

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

		static_vector<gfx_id, BACK_BUFFER_COUNT * 4> opaque_textures;
		static_vector<gfx_id, BACK_BUFFER_COUNT>	 depth_textures;

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
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

	void world_renderer::collect_model_instances()
	{
		const uint32	   mesh_instances_peak = _proxy_manager.get_mesh_instances_peak();
		auto&			   mesh_instances	   = _proxy_manager.get_mesh_instances();
		auto&			   entities			   = _proxy_manager.get_entities();
		chunk_allocator32& aux				   = _proxy_manager.get_aux();

		for (uint32 i = 0; i < mesh_instances_peak; i++)
		{
			const render_proxy_mesh_instance& mesh_instance = mesh_instances.get(i);
			if (mesh_instance.status != render_proxy_status::rps_active)
				continue;

			const render_proxy_entity& proxy_entity = entities.get(mesh_instance.entity);
			SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);

			if (proxy_entity.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
				continue;

			const render_proxy_mesh& proxy_mesh	  = _proxy_manager.get_mesh(mesh_instance.mesh);
			aabb					 frustum_aabb = proxy_mesh.local_aabb;

			const frustum_result res = frustum::test(_main_camera_view.view_frustum, proxy_mesh.local_aabb, proxy_entity.model.to_linear3x3(), proxy_entity.position);
			if (res == frustum_result::outside)
				continue;

			const uint32				  entity_index = create_gpu_entity({.model = proxy_entity.model});
			const render_proxy_model&	  proxy_model  = _proxy_manager.get_model(mesh_instance.model);
			const render_proxy_primitive* primitives   = aux.get<render_proxy_primitive>(proxy_mesh.primitives);
			const uint16*				  materials	   = aux.get<uint16>(proxy_model.materials);

			for (uint32 i = 0; i < proxy_mesh.primitive_count; i++)
			{
				const render_proxy_primitive& prim = primitives[i];

				const uint16 mat = materials[prim.material_index];

				create_gpu_object({
					.vertex_buffer = const_cast<buffer*>(&proxy_mesh.vertex_buffer),
					.index_buffer  = const_cast<buffer*>(&proxy_mesh.index_buffer),
					.vertex_start  = prim.vertex_start,
					.index_start   = prim.index_start,
					.index_count   = prim.index_count,
					.gpu_entity	   = entity_index,
					.material	   = mat,
					.is_skinned	   = proxy_mesh.is_skinned,
				});
			}
		}
	}

}
