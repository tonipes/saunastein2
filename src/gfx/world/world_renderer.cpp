// Copyright (c) 2025 Inan Evin

#include "world_renderer.hpp"
#include "math/math.hpp"

// gfx
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/world/gpu_entity.hpp"
#include "gfx/world/gpu_bone.hpp"
#include "gfx/world/gpu_light.hpp"
#include "gfx/world/gpu_shadow_data.hpp"
#include "gfx/world/renderable_collector.hpp"
#include "gfx/util/shadow_util.hpp"

#include <algorithm>
#include <execution>

namespace SFG
{
	world_renderer::world_renderer(proxy_manager& pm) : _proxy_manager(pm) {};

	void world_renderer::init(const vector2ui16& size, texture_queue* tq, buffer_queue* bq)
	{
		gfx_backend* backend = gfx_backend::get();

		_renderables.reserve(MAX_DRAW_CALLS);

		const bitmask16 color_flags = texture_flags::tf_is_2d | texture_flags::tf_render_target | texture_flags::tf_sampled;

		_base_size = size;

		// pfd
		for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd		 = _pfd[i];
			pfd.cmd_upload			 = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "wr_upload"});
			pfd.semp_frame.semaphore = backend->create_semaphore();

			pfd.shadow_data_buffer.create_staging_hw(
				{
					.size		= sizeof(gpu_shadow_data) * MAX_GPU_SHADOW_DATA,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "shadow_data_cpu",
				},
				{
					.size			 = sizeof(gpu_shadow_data) * MAX_GPU_SHADOW_DATA,
					.structure_size	 = sizeof(gpu_shadow_data),
					.structure_count = MAX_GPU_SHADOW_DATA,
					.flags			 = resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name		 = "shadow_data_gpu",
				});

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
					.debug_name		 = "lighting_spot_lights_gpu",
				});

			pfd.float_buffer.create_staging_hw(
				{
					.size		= sizeof(float) * 128,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "float_buffer",
				},
				{
					.size			 = sizeof(float) * 128,
					.structure_size	 = sizeof(float),
					.structure_count = 128,
					.flags			 = resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name		 = "float_buffer_gpu",
				});
		}

		_pass_pre_depth.init(size);
		_pass_opaque.init(size);
		_pass_lighting.init(size);
		_pass_shadows.init();
	}

	void world_renderer::uninit()
	{
		_pass_pre_depth.uninit();
		_pass_opaque.uninit();
		_pass_lighting.uninit();
		_pass_shadows.uninit();

		gfx_backend* backend = gfx_backend::get();

		for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_command_buffer(pfd.cmd_upload);
			backend->destroy_semaphore(pfd.semp_frame.semaphore);
			pfd.shadow_data_buffer.destroy();
			pfd.entity_buffer.destroy();
			pfd.bones_buffer.destroy();
			pfd.dir_lights_buffer.destroy();
			pfd.spot_lights_buffer.destroy();
			pfd.point_lights_buffer.destroy();
			pfd.float_buffer.destroy();
		}
	}

	void world_renderer::prepare(uint8 frame_index)
	{
		per_frame_data& pfd = _pfd[frame_index];
		pfd.reset();

		chunk_allocator32& proxy_aux = _proxy_manager.get_aux();

		// Handle main camera.
		const world_id main_cam_trait = _proxy_manager.get_main_camera();
		if (main_cam_trait != NULL_WORLD_ID)
		{
			const render_proxy_camera& cam_proxy  = _proxy_manager.get_camera(main_cam_trait);
			const render_proxy_entity& cam_entity = _proxy_manager.get_entity(cam_proxy.entity);

			const matrix4x4 view	  = matrix4x4::view(cam_entity.rotation, cam_entity.position);
			const matrix4x4 proj	  = matrix4x4::perspective_reverse_z(cam_proxy.fov_degrees, static_cast<float>(_base_size.x) / static_cast<float>(_base_size.y), cam_proxy.near_plane, cam_proxy.far_plane);
			const matrix4x4 view_proj = proj * view;
			_main_camera_view		  = {
						.view_frustum		  = frustum::extract(view_proj),
						.view_matrix		  = view,
						.proj_matrix		  = proj,
						.view_proj_matrix	  = view_proj,
						.inv_view_proj_matrix = view_proj.inverse(),
						.position			  = cam_entity.position,
						.near_plane			  = cam_proxy.near_plane,
						.far_plane			  = cam_proxy.far_plane,
						.fov_degrees		  = cam_proxy.fov_degrees,
			};

			if (cam_proxy.cascades.size != 0)
			{
				float* cascades_ptr = proxy_aux.get<float>(cam_proxy.cascades);
				for (uint8 i = 0; i < cam_proxy.cascade_count; i++)
					_main_camera_view.cascades.push_back(cascades_ptr[i]);
			}
		}

		// entities, lights, bones etc.
		// prepare shadow pass before, collect_and_upload will add passes to it.
		_pass_shadows.prepare(_proxy_manager, _main_camera_view, _base_size, frame_index);
		collect_and_upload(frame_index);

		_renderables.resize(0);
		renderable_collector::collect_model_instances(_proxy_manager, _main_camera_view, _renderables);
		_pass_pre_depth.prepare(_proxy_manager, _renderables, _main_camera_view, frame_index);
		_pass_opaque.prepare(_proxy_manager, _renderables, _main_camera_view, frame_index);
		_pass_lighting.prepare(_proxy_manager, _main_camera_view, frame_index);
	}

	void world_renderer::render(uint8 frame_index, gfx_id layout_global, gfx_id bind_group_global, uint64 prev_copy, uint64 next_copy, gfx_id sem_copy)
	{
		gfx_backend* backend   = gfx_backend::get();
		const gfx_id queue_gfx = backend->get_queue_gfx();

		per_frame_data&	  pfd		 = _pfd[frame_index];
		const vector2ui16 resolution = _base_size;

		const gfx_id cmd_depth		   = _pass_pre_depth.get_cmd_buffer(frame_index);
		const gfx_id cmd_opaque		   = _pass_opaque.get_cmd_buffer(frame_index);
		const gfx_id cmd_lighting	   = _pass_lighting.get_cmd_buffer(frame_index);
		const gfx_id cmd_shadows	   = _pass_shadows.get_cmd_buffer(frame_index);
		const gfx_id sem_frame		   = pfd.semp_frame.semaphore;
		const uint64 sem_frame_val	   = ++pfd.semp_frame.value;
		const uint16 shadow_pass_count = _pass_shadows.get_pass_count();

		const gfx_id											depth_texture				 = _pass_pre_depth.get_output_hw(frame_index);
		const gpu_index											gpu_index_depth_texture		 = _pass_pre_depth.get_output_gpu_index(frame_index);
		const gpu_index											gpu_index_entities			 = pfd.entity_buffer.get_gpu_index();
		const gpu_index											gpu_index_shadow_data_buffer = pfd.shadow_data_buffer.get_gpu_index();
		const gpu_index											gpu_index_bones				 = pfd.bones_buffer.get_gpu_index();
		const gpu_index											gpu_index_point_lights		 = pfd.point_lights_buffer.get_gpu_index();
		const gpu_index											gpu_index_spot_lights		 = pfd.spot_lights_buffer.get_gpu_index();
		const gpu_index											gpu_index_dir_lights		 = pfd.dir_lights_buffer.get_gpu_index();
		const gpu_index											gpu_index_float_buffer		 = pfd.float_buffer.get_gpu_index();
		const static_vector<gpu_index, GBUFFER_COLOR_TEXTURES>& gpu_index_gbuffer_textures	 = _pass_opaque.get_output_gpu_index(frame_index);

		static_vector<std::function<void()>, 10> tasks;

		tasks.push_back([&] {
			_pass_pre_depth.render({
				.frame_index		= frame_index,
				.size				= resolution,
				.gpu_index_entities = gpu_index_entities,
				.gpu_index_bones	= gpu_index_bones,
				.global_layout		= layout_global,
				.global_group		= bind_group_global,
			});
		});

		tasks.push_back([&] {
			_pass_shadows.render({
				.frame_index		= frame_index,
				.size				= resolution,
				.gpu_index_entities = gpu_index_entities,
				.gpu_index_bones	= gpu_index_bones,
				.global_layout		= layout_global,
				.global_group		= bind_group_global,
			});
		});

		tasks.push_back([&] {
			_pass_opaque.render({
				.frame_index		= frame_index,
				.size				= resolution,
				.gpu_index_entities = gpu_index_entities,
				.gpu_index_bones	= gpu_index_bones,
				.depth_texture		= depth_texture,
				.global_layout		= layout_global,
				.global_group		= bind_group_global,
			});
		});
		tasks.push_back([&] {
			_pass_lighting.render({
				.frame_index				  = frame_index,
				.size						  = resolution,
				.gpu_index_gbuffer_textures	  = gpu_index_gbuffer_textures,
				.gpu_index_depth_texture	  = gpu_index_depth_texture,
				.gpu_index_point_lights		  = gpu_index_point_lights,
				.gpu_index_spot_lights		  = gpu_index_spot_lights,
				.gpu_index_dir_lights		  = gpu_index_dir_lights,
				.gpu_index_entities			  = gpu_index_entities,
				.gpu_index_shadow_data_buffer = gpu_index_shadow_data_buffer,
				.gpu_index_float_buffer		  = gpu_index_float_buffer,
				.depth_texture				  = depth_texture,
				.global_layout				  = layout_global,
				.global_group				  = bind_group_global,
			});
		});

		std::for_each(std::execution::par, tasks.begin(), tasks.end(), [](auto&& task) { task(); });

		if (prev_copy != next_copy)
			backend->queue_wait(queue_gfx, &sem_copy, &next_copy, 1);

		// submit depth
		backend->submit_commands(queue_gfx, &cmd_depth, 1);

		// shadows
		if (shadow_pass_count != 0)
			backend->submit_commands(queue_gfx, &cmd_shadows, 1);

		// submit opaque, wait for depth
		backend->submit_commands(queue_gfx, &cmd_opaque, 1);

		// submit lighting
		backend->submit_commands(queue_gfx, &cmd_lighting, 1);
		backend->queue_signal(queue_gfx, &sem_frame, &sem_frame_val, 1);
	}

	void world_renderer::resize(const vector2ui16& size)
	{
		_base_size			 = size;
		gfx_backend* backend = gfx_backend::get();

		// depth prepass
		_pass_pre_depth.resize(size);
		_pass_opaque.resize(size);
		_pass_lighting.resize(size);
	}

	uint32 world_renderer::add_to_float_buffer(uint8 frame_index, float f)
	{
		per_frame_data& pfd = _pfd[frame_index];
		pfd.float_buffer.buffer_data(pfd._float_buffer_count * sizeof(float), &f, sizeof(float));
		pfd._float_buffer_count++;
		return pfd._float_buffer_count - 1;
	}

	void world_renderer::collect_and_upload(uint8 frame_index)
	{
		gfx_backend* backend   = gfx_backend::get();
		const gfx_id queue_gfx = backend->get_queue_gfx();

		per_frame_data& pfd = _pfd[frame_index];
		const gfx_id	cmd = pfd.cmd_upload;

		backend->reset_command_buffer(cmd);

		static_vector<barrier, 7> barriers;

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

		barriers.push_back({
			.resource	 = pfd.shadow_data_buffer.get_hw_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_copy_dest,
		});

		barriers.push_back({
			.resource	 = pfd.float_buffer.get_hw_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_copy_dest,
		});

		backend->cmd_barrier(cmd, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

		const uint8 cascades_count = static_cast<uint8>(_main_camera_view.cascades.size());
		for (uint8 i = 0; i < cascades_count; i++)
		{
			const uint32 idx = add_to_float_buffer(frame_index, _main_camera_view.cascades[i] * _main_camera_view.far_plane);
			if (i == 0)
				_main_camera_view.cascsades_gpu_index = idx;
		}

		if (pfd._float_buffer_count != 0)
			pfd.float_buffer.copy_region(cmd, 0, sizeof(float) * pfd._float_buffer_count);

		collect_and_upload_entities(cmd, frame_index);
		collect_and_upload_bones(cmd, frame_index);
		collect_and_upload_lights(cmd, frame_index);

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

		barriers.push_back({
			.resource	 = pfd.shadow_data_buffer.get_hw_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_ps_resource,
		});

		barriers.push_back({
			.resource	 = pfd.float_buffer.get_hw_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_ps_resource,
		});

		backend->cmd_barrier(cmd, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});
		backend->close_command_buffer(cmd);
		backend->submit_commands(queue_gfx, &cmd, 1);
	}

	void world_renderer::collect_and_upload_entities(gfx_id cmd_buffer, uint8 frame_index)
	{
		per_frame_data& pfd			  = _pfd[frame_index];
		auto&			entities	  = *_proxy_manager.get_entities();
		const uint32	entities_peak = _proxy_manager.get_peak_entities();

		size_t offset		  = 0;
		uint32 assigned_index = 0;

		for (uint32 i = 0; i < entities_peak; i++)
		{
			render_proxy_entity& e = entities.get(i);
			if (e.status != render_proxy_status::rps_active)
				continue;

			if (e.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
			{
				e._assigned_index = UINT32_MAX;
				continue;
			}

			e._assigned_index = assigned_index;

			const vector3	 forward = e.rotation.get_forward();
			const gpu_entity gpu_e	 = {
				  .model	= e.model.to_matrix4x4(),
				  .normal	= e.normal.to_matrix4x4(),
				  .position = vector4(e.position.x, e.position.y, e.position.z, 0),
				  .rotation = vector4(e.rotation.x, e.rotation.y, e.rotation.z, e.rotation.w),
				  .forward	= vector4(forward.x, forward.y, forward.z, 0.0f),
			  };

			pfd.entity_buffer.buffer_data(assigned_index * sizeof(gpu_entity), &gpu_e, sizeof(gpu_entity));
			assigned_index++;
		}

		if (assigned_index != 0)
			pfd.entity_buffer.copy_region(cmd_buffer, 0, assigned_index * sizeof(gpu_entity));
	}

	void world_renderer::collect_and_upload_bones(gfx_id cmd_buffer, uint8 frame_index)
	{
	}

	void world_renderer::collect_and_upload_lights(gfx_id cmd_buffer, uint8 frame_index)
	{
		per_frame_data& pfd = _pfd[frame_index];

		const uint32 points_peak  = _proxy_manager.get_peak_point_lights();
		const uint32 spots_peak	  = _proxy_manager.get_peak_spot_lights();
		const uint32 dirs_peak	  = _proxy_manager.get_peak_dir_lights();
		auto&		 spots		  = *_proxy_manager.get_spot_lights();
		auto&		 points		  = *_proxy_manager.get_point_lights();
		auto&		 dirs		  = *_proxy_manager.get_dir_lights();
		auto&		 entities	  = *_proxy_manager.get_entities();
		uint32		 points_count = 0, spots_count = 0, dirs_count = 0;
		uint32		 shadow_data_count = 0;

		const matrix4x4 main_view_matrix = _main_camera_view.view_matrix;
		const float		main_cam_near	 = _main_camera_view.near_plane;
		const float		main_cam_far	 = _main_camera_view.far_plane;
		const float		main_cam_fov	 = _main_camera_view.fov_degrees;
		const float		aspect_ratio	 = static_cast<float>(_base_size.x) / static_cast<float>(_base_size.y);

		for (uint32 i = 0; i < points_peak; i++)
		{
			render_proxy_point_light& light = points.get(i);
			if (light.status != render_proxy_status::rps_active)
				continue;

			const render_proxy_entity& proxy_entity = entities.get(light.entity);
			SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);

			if (proxy_entity.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
				continue;

			constexpr float energy_thresh = LIGHT_CULLING_ENERGY_THRESHOLD;
			const float		radius		  = math::sqrt(light.intensity / (4.0f * MATH_PI * energy_thresh));

			const frustum_result res = frustum::test(_main_camera_view.view_frustum, proxy_entity.position, radius);
			if (res == frustum_result::outside)
				continue;

			int32 first_shadow_index = -1;
			const float far_plane		   = math::almost_equal(light.range, 0.0f) ? main_cam_far : light.range;

			if (light.cast_shadows)
			{
				const float light_aspect = static_cast<float>(light.shadow_res.x) / static_cast<float>(light.shadow_res.y);
				first_shadow_index		 = static_cast<int32>(shadow_data_count);

				static_vector<vector3, 6> dirs;
				static_vector<vector3, 6> fws;

				dirs.push_back(vector3::right * 0.1);
				dirs.push_back(-vector3::right * 0.1);
				dirs.push_back(vector3::up * 0.1);
				dirs.push_back(-vector3::up * 0.1);
				dirs.push_back(vector3::forward * 0.1);
				dirs.push_back(-vector3::forward * 0.1);

				fws.push_back(vector3::up);
				fws.push_back(vector3::up);
				fws.push_back(vector3::forward);
				fws.push_back(vector3::forward);
				fws.push_back(vector3::up);
				fws.push_back(vector3::up);

				for (uint8 j = 0; j < 6; j++)
				{
					const matrix4x4 light_view		 = matrix4x4::look_at(proxy_entity.position, proxy_entity.position + dirs[j], fws[j]);
					const matrix4x4 light_projection = matrix4x4::perspective(90.5f, light_aspect, 0.01f, far_plane);

					const gpu_shadow_data sh = {
						.light_space_matrix = light_projection * light_view,
					};

					pfd.shadow_data_buffer.buffer_data(shadow_data_count * sizeof(gpu_shadow_data), &sh, sizeof(gpu_shadow_data));
					shadow_data_count++;

					_pass_shadows.add_pass({
						.pm				  = _proxy_manager,
						.frame_index	  = frame_index,
						.res			  = light.shadow_res,
						.texture		  = light.shadow_texture_hw[frame_index],
						.transition_owner = j == 0,
						.view_index		  = j,
						.proj			  = light_projection,
						.view			  = light_view,
						.position		  = proxy_entity.position,
						.cascade_near	  = main_cam_near,
						.cascade_far	  = far_plane,
						.fov			  = main_cam_fov,
					});
				}
			}

			const gpu_point_light data = {
				.color_entity_index			   = vector4(light.base_color.x, light.base_color.y, light.base_color.z, proxy_entity._assigned_index),
				.intensity_range			   = vector4(light.intensity, light.range, 0.0f, 0.0f),
				.shadow_res_map_and_data_index = vector4(light.shadow_res.x, light.shadow_res.y, static_cast<float>(light.shadow_texture_gpu_index[frame_index]), static_cast<float>(first_shadow_index)),
				.far_plane					   = far_plane,
			};

			pfd.point_lights_buffer.buffer_data(points_count * sizeof(gpu_point_light), &data, sizeof(gpu_point_light));
			points_count++;
		}

		for (uint32 i = 0; i < spots_peak; i++)
		{
			render_proxy_spot_light& light = spots.get(i);
			if (light.status != render_proxy_status::rps_active)
				continue;

			const render_proxy_entity& proxy_entity = entities.get(light.entity);
			SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);
			if (proxy_entity.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
				continue;

			constexpr float energy_thresh = LIGHT_CULLING_ENERGY_THRESHOLD;
			const float		radius		  = math::sqrt(light.intensity / (4.0f * MATH_PI * energy_thresh));

			const frustum_result res = frustum::test(_main_camera_view.view_frustum, proxy_entity.position, radius);
			if (res == frustum_result::outside)
				continue;

			const float cos_outer = math::cos(light.outer_cone);

			int32 shadow_data_index = -1;
			if (light.cast_shadows)
			{
				const float far_plane	 = math::almost_equal(light.range, 0.0f) ? main_cam_far : light.range;
				const float fov_deg		 = RAD_2_DEG * 2.0f * math::acos(cos_outer);
				const float light_aspect = static_cast<float>(light.shadow_res.x) / static_cast<float>(light.shadow_res.y);

				const matrix4x4 light_view		 = matrix4x4::look_at(proxy_entity.position, proxy_entity.position + proxy_entity.rotation.get_forward() * 0.1, vector3::up);
				const matrix4x4 light_projection = matrix4x4::perspective(fov_deg, light_aspect, 0.01f, far_plane);

				const gpu_shadow_data sh = {
					.light_space_matrix = light_projection * light_view,
				};

				pfd.shadow_data_buffer.buffer_data(shadow_data_count * sizeof(gpu_shadow_data), &sh, sizeof(gpu_shadow_data));

				_pass_shadows.add_pass({
					.pm				  = _proxy_manager,
					.frame_index	  = frame_index,
					.res			  = light.shadow_res,
					.texture		  = light.shadow_texture_hw[frame_index],
					.transition_owner = true,
					.view_index		  = 0,
					.proj			  = light_projection,
					.view			  = light_view,
					.position		  = proxy_entity.position,
					.cascade_near	  = main_cam_near,
					.cascade_far	  = far_plane,
					.fov			  = main_cam_fov,
				});

				shadow_data_index = static_cast<int32>(shadow_data_count);
				shadow_data_count++;
			}

			const gpu_spot_light data = {
				.color_entity_index			   = vector4(light.base_color.x, light.base_color.y, light.base_color.z, proxy_entity._assigned_index),
				.intensity_range_inner_outer   = vector4(light.intensity, light.range, math::cos(light.inner_cone), cos_outer),
				.shadow_res_map_and_data_index = vector4(light.shadow_res.x, light.shadow_res.y, static_cast<float>(light.shadow_texture_gpu_index[frame_index]), static_cast<float>(shadow_data_index)),
			};

			pfd.spot_lights_buffer.buffer_data(sizeof(gpu_spot_light) * spots_count, &data, sizeof(gpu_spot_light));
			spots_count++;
		}

		const static_vector<float, MAX_SHADOW_CASCADES>& cascade_levels = _main_camera_view.cascades;

		for (uint32 i = 0; i < dirs_peak; i++)
		{
			render_proxy_dir_light& light = dirs.get(i);
			if (light.status != render_proxy_status::rps_active)
				continue;

			const render_proxy_entity& proxy_entity = entities.get(light.entity);
			SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);

			if (proxy_entity.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
				continue;

			int32 first_shadow_index = -1;

			if (light.cast_shadows)
			{
				first_shadow_index = static_cast<int32>(shadow_data_count);

				const uint8 cascades = static_cast<uint8>(cascade_levels.size()) + 1;
				for (uint8 j = 0; j < cascades; j++)
				{
					const bool	last		   = j == cascades - 1;
					const float far_multiplier = !last ? cascade_levels[j] : 1.0f;
					const float cascade_far	   = main_cam_far * far_multiplier;
					const float cascade_near   = j == 0 ? main_cam_near : cascade_levels[static_cast<int32>(j) - 1] * main_cam_far;

					const matrix4x4 proj	  = matrix4x4::perspective(main_cam_fov, aspect_ratio, cascade_near, cascade_far);
					const matrix4x4 view_proj = proj * main_view_matrix;

					static_vector<vector4, 8> frustum_world_space;
					vector3					  frustum_world_center;
					shadow_util::get_world_space_ndc(view_proj.inverse(), frustum_world_space, frustum_world_center);

					const matrix4x4 light_view = matrix4x4::look_at(frustum_world_center, frustum_world_center + proxy_entity.rotation.get_forward(), vector3::up);

					matrix4x4 light_projection = matrix4x4::identity;
					vector2	  texel_world	   = vector2::zero;
					shadow_util::get_lightspace_projection(light_projection, light_view, frustum_world_space, light.shadow_res, texel_world);

					const gpu_shadow_data sh = {
						.light_space_matrix = light_projection * light_view,
						.texel_world		= math::max(texel_world.x, texel_world.y),
					};

					pfd.shadow_data_buffer.buffer_data(shadow_data_count * sizeof(gpu_shadow_data), &sh, sizeof(gpu_shadow_data));

					_pass_shadows.add_pass({
						.pm				  = _proxy_manager,
						.frame_index	  = frame_index,
						.res			  = light.shadow_res,
						.texture		  = light.shadow_texture_hw[frame_index],
						.transition_owner = j == 0,
						.view_index		  = j,
						.proj			  = light_projection,
						.view			  = light_view,
						.position		  = frustum_world_center,
						.cascade_near	  = cascade_near,
						.cascade_far	  = cascade_far,
						.fov			  = main_cam_fov,
					});
					shadow_data_count++;
				}
			}

			const gpu_dir_light data = {
				.color_entity_index			   = vector4(light.base_color.x, light.base_color.y, light.base_color.z, proxy_entity._assigned_index),
				.intensity					   = vector4(light.intensity, 0.0f, 0.0f, 0.0f),
				.shadow_res_map_and_data_index = vector4(light.shadow_res.x, light.shadow_res.y, static_cast<float>(light.shadow_texture_gpu_index[frame_index]), static_cast<float>(first_shadow_index)),
			};

			pfd.dir_lights_buffer.buffer_data(sizeof(gpu_dir_light) * dirs_count, &data, sizeof(gpu_dir_light));
			dirs_count++;
		}

		if (dirs_count != 0)
			pfd.dir_lights_buffer.copy_region(cmd_buffer, 0, dirs_count * sizeof(gpu_dir_light));
		if (spots_count != 0)
			pfd.spot_lights_buffer.copy_region(cmd_buffer, 0, spots_count * sizeof(gpu_spot_light));
		if (points_count != 0)
			pfd.point_lights_buffer.copy_region(cmd_buffer, 0, points_count * sizeof(gpu_point_light));

		if (shadow_data_count != 0)
			pfd.shadow_data_buffer.copy_region(cmd_buffer, 0, shadow_data_count * sizeof(gpu_shadow_data));
	}

}
