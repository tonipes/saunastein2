// Copyright (c) 2025 Inan Evin

#include "world_renderer.hpp"
#include "common/system_info.hpp"
#include "memory/memory_tracer.hpp"
#include "math/math.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/texture_queue.hpp"
#include "gfx/buffer_queue.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "world/world.hpp"
#include "world/traits/trait_light.hpp"
#include "world/traits/trait_mesh_instance.hpp"
#include "resources/mesh.hpp"
#include "resources/primitive.hpp"

#include <algorithm>
#include <execution>

namespace SFG
{
#define SHARED_BUMP_ALLOC_SZ 1024 * 512

	world_renderer::world_renderer(proxy_manager& pm, world& w) : _proxy_manager(pm), _world(w) {};

	void world_renderer::init(const vector2ui16& size, texture_queue* tq, buffer_queue* bq)
	{
		_texture_queue = tq;
		_buffer_queue  = bq;

		gfx_backend* backend = gfx_backend::get();

		const bitmask16 color_flags = texture_flags::tf_is_2d | texture_flags::tf_render_target | texture_flags::tf_sampled;

		_base_size = size;

		static_vector<gpu_index, BACK_BUFFER_COUNT> entities_ptr;
		static_vector<gpu_index, BACK_BUFFER_COUNT> bones_ptr;
		static_vector<gpu_index, BACK_BUFFER_COUNT> dir_lights_ptr;
		static_vector<gpu_index, BACK_BUFFER_COUNT> point_lights_ptr;
		static_vector<gpu_index, BACK_BUFFER_COUNT> spot_lights_ptr;

		// pfd
		for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd		 = _pfd[i];
			pfd.cmd_upload			 = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "wr_upload"});
			pfd.semp_frame.semaphore = backend->create_semaphore();

			pfd.bones_buffer.create_staging_hw(
				{
					.size		= sizeof(gpu_bone) * MAX_GPU_BONES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "opaque_bones_cpu",
				},
				{
					.size			 = sizeof(gpu_bone) * MAX_GPU_BONES,
					.structure_size	 = sizeof(gpu_bone),
					.structure_count = MAX_GPU_BONES,
					.flags			 = resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name		 = "opaque_bones_gpu",
				});

			pfd.entity_buffer.create_staging_hw(
				{
					.size		= sizeof(gpu_entity) * MAX_GPU_ENTITIES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "opaque_entities_cpu",
				},
				{
					.size			 = sizeof(gpu_entity) * MAX_GPU_ENTITIES,
					.structure_size	 = sizeof(gpu_entity),
					.structure_count = MAX_GPU_ENTITIES,
					.flags			 = resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name		 = "opaque_entities_gpu",
				});

			pfd.dir_lights_buffer.create_staging_hw(
				{
					.size		= sizeof(gpu_dir_light) * MAX_WORLD_TRAIT_DIR_LIGHTS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "lighting_dir_lights_cpu",
				},
				{
					.size			 = sizeof(gpu_dir_light) * MAX_WORLD_TRAIT_DIR_LIGHTS,
					.structure_size	 = sizeof(gpu_dir_light),
					.structure_count = MAX_WORLD_TRAIT_DIR_LIGHTS,
					.flags			 = resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name		 = "lighting_dir_lights_gpu",
				});

			pfd.point_lights_buffer.create_staging_hw(
				{
					.size		= sizeof(gpu_point_light) * MAX_WORLD_TRAIT_POINT_LIGHTS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "lighting_point_lights_cpu",
				},
				{
					.size			 = sizeof(gpu_point_light) * MAX_WORLD_TRAIT_POINT_LIGHTS,
					.structure_size	 = sizeof(gpu_point_light),
					.structure_count = MAX_WORLD_TRAIT_POINT_LIGHTS,
					.flags			 = resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name		 = "lighting_point_lights_gpu",
				});

			pfd.spot_lights_buffer.create_staging_hw(
				{
					.size		= sizeof(gpu_spot_light) * MAX_WORLD_TRAIT_SPOT_LIGHTS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "lighting_spot_lights_cpu",
				},
				{
					.size			 = sizeof(gpu_spot_light) * MAX_WORLD_TRAIT_SPOT_LIGHTS,
					.structure_size	 = sizeof(gpu_spot_light),
					.structure_count = MAX_WORLD_TRAIT_SPOT_LIGHTS,
					.flags			 = resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name		 = "lighting_spot_lights_cpu",
				});

			entities_ptr.push_back(pfd.entity_buffer.get_gpu_heap_index());
			bones_ptr.push_back(pfd.bones_buffer.get_gpu_heap_index());
			dir_lights_ptr.push_back(pfd.dir_lights_buffer.get_gpu_heap_index());
			point_lights_ptr.push_back(pfd.point_lights_buffer.get_gpu_heap_index());
			spot_lights_ptr.push_back(pfd.spot_lights_buffer.get_gpu_heap_index());
		}

		// Command allocations
		{
			const size_t total_shared_size = SHARED_BUMP_ALLOC_SZ;
			const size_t lane_count		   = 4;
			const size_t size_per_lane	   = total_shared_size / lane_count;
			_shared_command_alloc		   = (uint8*)SFG_MALLOC(total_shared_size);
			PUSH_ALLOCATION_SZ(total_shared_size);

			uint8* alloc_head = _shared_command_alloc;

			_pass_pre_depth.init({
				.size		= size,
				.alloc		= alloc_head,
				.alloc_size = size_per_lane,
				.entities	= entities_ptr.data(),
				.bones		= bones_ptr.data(),
			});
			alloc_head += size_per_lane;

			static_vector<gfx_id, BACK_BUFFER_COUNT>	depths_hw;
			static_vector<gpu_index, BACK_BUFFER_COUNT> depths_index;
			for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
			{
				depths_hw.push_back(_pass_pre_depth.get_output_hw(i));
				depths_index.push_back(_pass_pre_depth.get_output_gpu_index(i));
			}

			_pass_opaque.init({
				.size			= size,
				.alloc			= alloc_head,
				.alloc_size		= size_per_lane,
				.entities		= entities_ptr.data(),
				.bones			= bones_ptr.data(),
				.depth_textures = depths_hw.data(),
			});
			alloc_head += size_per_lane;

			static_vector<gpu_index, BACK_BUFFER_COUNT * GBUFFER_COLOR_TEXTURES> gbuffer_textures;
			for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
			{
				for (uint32 j = 0; j < GBUFFER_COLOR_TEXTURES; j++)
				{
					gbuffer_textures.push_back(_pass_opaque.get_output_gpu_index(i, j));
				}
			}

			_pass_lighting.init({
				.size			   = size,
				.alloc			   = alloc_head,
				.alloc_size		   = size_per_lane,
				.entities		   = entities_ptr.data(),
				.point_lights	   = point_lights_ptr.data(),
				.spot_lights	   = spot_lights_ptr.data(),
				.dir_lights		   = dir_lights_ptr.data(),
				.gbuffer_textures  = gbuffer_textures.data(),
				.depth_textures	   = depths_index.data(),
				.depth_textures_hw = depths_hw.data(),
			});
			alloc_head += size_per_lane;

			_pass_shadows.init({
				.size		= size,
				.alloc		= alloc_head,
				.alloc_size = size_per_lane,
				.entities	= entities_ptr.data(),
				.bones		= bones_ptr.data(),
			});
			alloc_head += size_per_lane;
		}
	}

	void world_renderer::uninit()
	{
		_pass_pre_depth.uninit();
		_pass_opaque.uninit();
		_pass_lighting.uninit();
		_pass_shadows.uninit();
		// _pass_post_combiner.uninit();

		PUSH_DEALLOCATION_SZ(SHARED_BUMP_ALLOC_SZ);
		SFG_FREE(_shared_command_alloc);

		gfx_backend* backend = gfx_backend::get();

		for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_command_buffer(pfd.cmd_upload);
			backend->destroy_semaphore(pfd.semp_frame.semaphore);
			pfd.entity_buffer.destroy();
			pfd.bones_buffer.destroy();
			pfd.dir_lights_buffer.destroy();
			pfd.spot_lights_buffer.destroy();
			pfd.point_lights_buffer.destroy();
		}
	}

	void world_renderer::prepare(uint8 frame_index)
	{
		per_frame_data& pfd = _pfd[frame_index];
		_main_renderable_collector.reset();

		// Handle main camera.
		const world_id main_cam_trait = _proxy_manager.get_main_camera();
		if (main_cam_trait != NULL_WORLD_ID)
		{
			const render_proxy_camera& cam_proxy  = _proxy_manager.get_camera(main_cam_trait);
			const render_proxy_entity& cam_entity = _proxy_manager.get_entity(cam_proxy.entity);
			_main_renderable_collector.generate_view_from_cam(cam_entity, cam_proxy, _base_size);
		}

		_main_renderable_collector.collect_entities(_proxy_manager);
		_main_renderable_collector.collect_model_instances(_proxy_manager);
		_main_renderable_collector.collect_lights(_proxy_manager);

		_pass_pre_depth.prepare(_proxy_manager, _main_renderable_collector, frame_index);
		_pass_opaque.prepare(_proxy_manager, _main_renderable_collector, frame_index);
		_pass_lighting.prepare(_proxy_manager, _main_renderable_collector, frame_index);
		_pass_shadows.prepare(_proxy_manager, frame_index);
	}

	void world_renderer::render(uint8 frame_index, gfx_id layout_global, gfx_id bind_group_global, uint64 prev_copy, uint64 next_copy, gfx_id sem_copy)
	{
		gfx_backend* backend   = gfx_backend::get();
		const gfx_id queue_gfx = backend->get_queue_gfx();

		per_frame_data&	  pfd		 = _pfd[frame_index];
		const vector2ui16 resolution = _base_size;

		const gfx_id  cmd_depth			= _pass_pre_depth.get_cmd_buffer(frame_index);
		const gfx_id  cmd_opaque		= _pass_opaque.get_cmd_buffer(frame_index);
		const gfx_id  cmd_lighting		= _pass_lighting.get_cmd_buffer(frame_index);
		const gfx_id  cmd_post			= _pass_post_combiner.get_cmd_buffer(frame_index);
		const gfx_id* cmd_shadows		= _pass_shadows.get_cmd_buffers(frame_index);
		const uint8	  cmd_shadows_count = _pass_shadows.get_active_cmd_buffers_count(frame_index);
		const gfx_id  sem_frame			= pfd.semp_frame.semaphore;
		const uint64  sem_frame_val		= ++pfd.semp_frame.value;

		upload(frame_index);

		static_vector<std::function<void()>, 10> tasks;

		tasks.push_back([&] {
			_pass_pre_depth.render({
				.frame_index   = frame_index,
				.size		   = resolution,
				.global_layout = layout_global,
				.global_group  = bind_group_global,
			});
		});

		for (uint8 i = 0; i < cmd_shadows_count; i++)
		{
			tasks.push_back([&] {
				_pass_shadows.render({
					.cmd_index	   = i,
					.frame_index   = frame_index,
					.size		   = resolution,
					.global_layout = layout_global,
					.global_group  = bind_group_global,
				});
			});
		}

		tasks.push_back([&] {
			_pass_opaque.render({
				.frame_index   = frame_index,
				.size		   = resolution,
				.global_layout = layout_global,
				.global_group  = bind_group_global,
			});
		});
		tasks.push_back([&] {
			_pass_lighting.render({
				.frame_index   = frame_index,
				.size		   = resolution,
				.global_layout = layout_global,
				.global_group  = bind_group_global,
			});
		});

		std::for_each(std::execution::par, tasks.begin(), tasks.end(), [](auto&& task) { task(); });

		if (prev_copy != next_copy)
			backend->queue_wait(queue_gfx, &sem_copy, &next_copy, 1);

		// submit depth
		backend->submit_commands(queue_gfx, &cmd_depth, 1);
		// backend->queue_signal(queue_gfx, &sem_depth, &sem_depth_val, 1);

		// if (cmd_shadows_count != 0)
		// 	backend->submit_commands(queue_gfx, cmd_shadows, cmd_shadows_count);

		// submit opaque, wait for depth
		// backend->queue_wait(queue_gfx, &sem_depth, &sem_depth_val, 1);
		backend->submit_commands(queue_gfx, &cmd_opaque, 1);
		// backend->queue_signal(queue_gfx, &sem_opaque, &sem_opaque_val, 1);

		// submit lighting, wait opaque
		// backend->queue_wait(queue_gfx, &sem_opaque, &sem_opaque_val, 1);
		backend->submit_commands(queue_gfx, &cmd_lighting, 1);
		backend->queue_signal(queue_gfx, &sem_frame, &sem_frame_val, 1);
	}

	void world_renderer::resize(const vector2ui16& size)
	{
		_base_size			 = size;
		gfx_backend* backend = gfx_backend::get();

		// depth prepass
		_pass_pre_depth.resize(size);

		// gbuffer
		static_vector<gfx_id, BACK_BUFFER_COUNT>	depths;
		static_vector<gpu_index, BACK_BUFFER_COUNT> depths_index;
		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			depths.push_back(_pass_pre_depth.get_output_hw(i));
			depths_index.push_back(_pass_pre_depth.get_output_gpu_index(i));
		}

		_pass_opaque.resize(size, depths.data());

		// lighting
		static_vector<gpu_index, BACK_BUFFER_COUNT * GBUFFER_COLOR_TEXTURES> gbuffer_textures;
		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			for (uint32 j = 0; j < GBUFFER_COLOR_TEXTURES; j++)
			{
				gbuffer_textures.push_back(_pass_opaque.get_output_gpu_index(i, j));
			}
		}

		_pass_lighting.resize(size, gbuffer_textures.data(), depths_index.data(), depths.data());
	}

	void world_renderer::upload(uint8 frame_index)
	{
		gfx_backend*		  backend	   = gfx_backend::get();
		per_frame_data&		  pfd		   = _pfd[frame_index];
		renderable_collector& collector	   = _main_renderable_collector;
		const auto&			  entities	   = collector.get_entities();
		const auto&			  bones		   = collector.get_bones();
		const auto&			  dir_lights   = collector.get_dir_lights();
		const auto&			  spot_lights  = collector.get_spot_lights();
		const auto&			  point_lights = collector.get_point_lights();

		const gfx_id queue_gfx = backend->get_queue_gfx();
		const gfx_id cmd	   = pfd.cmd_upload;

		backend->reset_command_buffer(cmd);

		static_vector<barrier, 20> barriers;

		barriers.push_back({
			.resource	 = pfd.bones_buffer.get_hw_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_copy_dest,
		});

		barriers.push_back({
			.resource	 = pfd.entity_buffer.get_hw_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_copy_dest,
		});

		barriers.push_back({
			.resource	 = pfd.dir_lights_buffer.get_hw_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_copy_dest,
		});

		barriers.push_back({
			.resource	 = pfd.spot_lights_buffer.get_hw_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_copy_dest,
		});

		barriers.push_back({
			.resource	 = pfd.point_lights_buffer.get_hw_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_copy_dest,
		});

		backend->cmd_barrier(cmd, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

		if (!entities.empty())
		{
			pfd.entity_buffer.buffer_data(0, entities.data(), entities.size() * sizeof(gpu_entity));
			pfd.entity_buffer.copy_region(cmd, 0, entities.size() * sizeof(gpu_entity));
		}

		if (!bones.empty())
		{
			pfd.bones_buffer.buffer_data(0, bones.data(), bones.size() * sizeof(gpu_bone));
			pfd.bones_buffer.copy_region(cmd, 0, bones.size() * sizeof(gpu_bone));
		}

		if (!dir_lights.empty())
		{
			pfd.dir_lights_buffer.buffer_data(0, dir_lights.data(), dir_lights.size() * sizeof(gpu_dir_light));
			pfd.dir_lights_buffer.copy_region(cmd, 0, dir_lights.size() * sizeof(gpu_dir_light));
		}

		if (!point_lights.empty())
		{
			pfd.point_lights_buffer.buffer_data(0, point_lights.data(), point_lights.size() * sizeof(gpu_point_light));
			pfd.point_lights_buffer.copy_region(cmd, 0, point_lights.size() * sizeof(gpu_point_light));
		}

		if (!spot_lights.empty())
		{
			pfd.spot_lights_buffer.buffer_data(0, spot_lights.data(), spot_lights.size() * sizeof(gpu_spot_light));
			pfd.spot_lights_buffer.copy_region(cmd, 0, spot_lights.size() * sizeof(gpu_spot_light));
		}

		barriers.resize(0);

		barriers.push_back({
			.resource	 = pfd.bones_buffer.get_hw_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_non_ps_resource,
		});

		barriers.push_back({
			.resource	 = pfd.entity_buffer.get_hw_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_non_ps_resource,
		});

		barriers.push_back({
			.resource	 = pfd.dir_lights_buffer.get_hw_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_ps_resource,
		});

		barriers.push_back({
			.resource	 = pfd.spot_lights_buffer.get_hw_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_ps_resource,
		});

		barriers.push_back({
			.resource	 = pfd.point_lights_buffer.get_hw_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_ps_resource,
		});

		backend->cmd_barrier(cmd, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});
		backend->close_command_buffer(cmd);
		backend->submit_commands(queue_gfx, &cmd, 1);
	}

}
