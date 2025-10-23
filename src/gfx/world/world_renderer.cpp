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
#include "world/world.hpp"
#include "world/traits/trait_light.hpp"
#include "world/traits/trait_mesh_instance.hpp"
#include "resources/mesh.hpp"
#include "resources/primitive.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include <algorithm>
#include <execution>

/* DEBUG */
#include "serialization/serialization.hpp"

namespace SFG
{
#define RT_FORMAT			 format::r8g8b8a8_srgb
#define DEPTH_FORMAT		 format::d16_unorm
#define SHARED_BUMP_ALLOC_SZ 1024 * 512

	world_renderer::world_renderer(proxy_manager& pm, world& w) : _proxy_manager(pm), _world(w) {};

	void world_renderer::init(const vector2ui16& size, texture_queue* tq, buffer_queue* bq)
	{
		_texture_queue = tq;
		_buffer_queue  = bq;

		gfx_backend* backend = gfx_backend::get();

		const bitmask16 color_flags = texture_flags::tf_is_2d | texture_flags::tf_render_target | texture_flags::tf_sampled;

		_base_size = size;

		static_vector<gfx_id, BACK_BUFFER_COUNT> entities_ptr;
		static_vector<gfx_id, BACK_BUFFER_COUNT> bones_ptr;
		static_vector<gfx_id, BACK_BUFFER_COUNT> dir_lights_ptr;
		static_vector<gfx_id, BACK_BUFFER_COUNT> point_lights_ptr;
		static_vector<gfx_id, BACK_BUFFER_COUNT> spot_lights_ptr;

		// pfd
		for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			pfd.cmd_upload		= backend->create_command_buffer({.type = command_type::graphics, .debug_name = "wr_upload"});

			pfd.bones_buffer.create_staging_hw(
				{
					.size		= sizeof(gpu_bone) * MAX_GPU_BONES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "opaque_bones_cpu",
				},
				{
					.size		= sizeof(gpu_bone) * MAX_GPU_BONES,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name = "opaque_bones_gpu",
				});

			pfd.entity_buffer.create_staging_hw(
				{
					.size		= sizeof(gpu_entity) * MAX_GPU_ENTITIES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "opaque_entities_cpu",
				},
				{
					.size		= sizeof(gpu_entity) * MAX_GPU_ENTITIES,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name = "opaque_entities_gpu",
				});

			pfd.dir_lights_buffer.create_staging_hw(
				{
					.size		= sizeof(gpu_dir_light) * MAX_WORLD_TRAIT_DIR_LIGHTS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "lighting_dir_lights_cpu",
				},
				{
					.size		= sizeof(gpu_dir_light) * MAX_WORLD_TRAIT_DIR_LIGHTS,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name = "lighting_dir_lights_gpu",
				});

			pfd.point_lights_buffer.create_staging_hw(
				{
					.size		= sizeof(gpu_point_light) * MAX_WORLD_TRAIT_POINT_LIGHTS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "lighting_point_lights_cpu",
				},
				{
					.size		= sizeof(gpu_point_light) * MAX_WORLD_TRAIT_POINT_LIGHTS,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name = "lighting_point_lights_gpu",
				});

			pfd.spot_lights_buffer.create_staging_hw(
				{
					.size		= sizeof(gpu_spot_light) * MAX_WORLD_TRAIT_SPOT_LIGHTS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "lighting_spot_lights_cpu",
				},
				{
					.size		= sizeof(gpu_spot_light) * MAX_WORLD_TRAIT_SPOT_LIGHTS,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name = "lighting_spot_lights_cpu",
				});

			entities_ptr.push_back(pfd.entity_buffer.get_hw_gpu());
			bones_ptr.push_back(pfd.bones_buffer.get_hw_gpu());
			dir_lights_ptr.push_back(pfd.dir_lights_buffer.get_hw_gpu());
			point_lights_ptr.push_back(pfd.point_lights_buffer.get_hw_gpu());
			spot_lights_ptr.push_back(pfd.spot_lights_buffer.get_hw_gpu());
		}

		// Command allocations
		{
			const size_t total_shared_size = SHARED_BUMP_ALLOC_SZ;
			const size_t lane_count		   = 3;
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

			static_vector<gfx_id, BACK_BUFFER_COUNT> depth_textures;
			for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
				depth_textures.push_back(_pass_pre_depth.get_depth_texture(i));

			_pass_opaque.init({
				.size			= size,
				.alloc			= alloc_head,
				.alloc_size		= size_per_lane,
				.entities		= entities_ptr.data(),
				.bones			= bones_ptr.data(),
				.depth_textures = depth_textures.data(),
			});
			alloc_head += size_per_lane;

			static_vector<gfx_id, BACK_BUFFER_COUNT * 4> gbuffer_textures;
			for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
			{
				gbuffer_textures.push_back(_pass_opaque.get_color_texture(i, 0));
				gbuffer_textures.push_back(_pass_opaque.get_color_texture(i, 1));
				gbuffer_textures.push_back(_pass_opaque.get_color_texture(i, 2));
				gbuffer_textures.push_back(_pass_opaque.get_color_texture(i, 3));
			}

			_pass_lighting.init({
				.size			  = size,
				.alloc			  = alloc_head,
				.alloc_size		  = size_per_lane,
				.entities		  = entities_ptr.data(),
				.point_lights	  = point_lights_ptr.data(),
				.spot_lights	  = spot_lights_ptr.data(),
				.dir_lights		  = dir_lights_ptr.data(),
				.gbuffer_textures = gbuffer_textures.data(),
				.depth_textures	  = depth_textures.data(),
			});

			alloc_head += size_per_lane;
		}
	}

	void world_renderer::uninit()
	{
		_pass_pre_depth.uninit();
		_pass_opaque.uninit();
		_pass_lighting.uninit();
		// _pass_post_combiner.uninit();

		PUSH_DEALLOCATION_SZ(SHARED_BUMP_ALLOC_SZ);
		SFG_FREE(_shared_command_alloc);

		gfx_backend* backend = gfx_backend::get();

		world_render_data& rd = _render_data;

		for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_command_buffer(pfd.cmd_upload);
			pfd.entity_buffer.destroy();
			pfd.bones_buffer.destroy();
			pfd.dir_lights_buffer.destroy();
			pfd.spot_lights_buffer.destroy();
			pfd.point_lights_buffer.destroy();
		}
	}

	void world_renderer::prepare(uint8 frame_index)
	{
		per_frame_data&	   pfd = _pfd[frame_index];
		world_render_data& rd  = _render_data;
		rd.reset();

		// Handle main camera.
		const world_id main_cam_trait = _proxy_manager.get_main_camera();
		_main_camera_view			  = {};
		if (main_cam_trait != NULL_WORLD_ID)
		{
			const render_proxy_camera& cam_proxy  = _proxy_manager.get_camera(main_cam_trait);
			const render_proxy_entity& cam_entity = _proxy_manager.get_entity(cam_proxy.entity);
			const matrix4x3&		   cam_abs	  = cam_entity.model;
			_main_camera_view					  = {
									.view_matrix = matrix4x4::view(cam_entity.rotation, cam_entity.position),
									.proj_matrix = matrix4x4::perspective_reverse_z(cam_proxy.fov_degrees, static_cast<float>(_base_size.x) / static_cast<float>(_base_size.y), cam_proxy.near_plane, cam_proxy.far_plane),
									.position	 = cam_entity.position,
			};
		}
		_main_camera_view.view_proj_matrix	   = _main_camera_view.proj_matrix * _main_camera_view.view_matrix;
		_main_camera_view.inv_view_proj_matrix = _main_camera_view.view_proj_matrix.inverse();
		_main_camera_view.view_frustum		   = frustum::extract(_main_camera_view.view_proj_matrix);

		collect_model_instances();
		collect_lights();

		if (!rd.entities.empty())
			pfd.entity_buffer.buffer_data(0, rd.entities.data(), rd.entities.size() * sizeof(gpu_entity));

		if (!rd.bones.empty())
			pfd.bones_buffer.buffer_data(0, rd.bones.data(), rd.bones.size() * sizeof(gpu_bone));

		if (!rd.dir_lights.empty())
			pfd.dir_lights_buffer.buffer_data(0, rd.dir_lights.data(), rd.dir_lights.size() * sizeof(gpu_dir_light));

		if (!rd.point_lights.empty())
			pfd.point_lights_buffer.buffer_data(0, rd.point_lights.data(), rd.point_lights.size() * sizeof(gpu_point_light));

		if (!rd.spot_lights.empty())
			pfd.spot_lights_buffer.buffer_data(0, rd.spot_lights.data(), rd.spot_lights.size() * sizeof(gpu_spot_light));

		_pass_pre_depth.prepare(_proxy_manager, _main_camera_view, frame_index, _render_data);
		_pass_opaque.prepare(_proxy_manager, _main_camera_view, frame_index, rd);
		_pass_lighting.prepare(_proxy_manager, _main_camera_view, frame_index);
	}

	void world_renderer::render(uint8 frame_index, gfx_id layout_global, gfx_id bind_group_global, uint64 prev_copy, uint64 next_copy, gfx_id sem_copy)
	{
		gfx_backend* backend   = gfx_backend::get();
		const gfx_id queue_gfx = backend->get_queue_gfx();

		per_frame_data&	   pfd		  = _pfd[frame_index];
		world_render_data& rd		  = _render_data;
		const vector2ui16  resolution = _base_size;

		semaphore_data& sd_depth		 = _pass_pre_depth.get_semaphore(frame_index);
		semaphore_data& sd_opaque		 = _pass_opaque.get_semaphore(frame_index);
		semaphore_data& sd_lighting		 = _pass_lighting.get_semaphore(frame_index);
		semaphore_data& sd_post			 = _pass_post_combiner.get_semaphore(frame_index);
		const gfx_id	sem_depth		 = sd_depth.semaphore;
		const gfx_id	sem_opaque		 = sd_opaque.semaphore;
		const gfx_id	sem_lighting	 = sd_lighting.semaphore;
		const gfx_id	sem_post		 = sd_post.semaphore;
		const uint64	sem_depth_val	 = ++sd_depth.value;
		const uint64	sem_opaque_val	 = ++sd_opaque.value;
		const uint64	sem_lighting_val = ++sd_lighting.value;
		const uint64	sem_post_val	 = ++sd_post.value;
		const gfx_id	cmd_depth		 = _pass_pre_depth.get_cmd_buffer(frame_index);
		const gfx_id	cmd_opaque		 = _pass_opaque.get_cmd_buffer(frame_index);
		const gfx_id	cmd_lighting	 = _pass_lighting.get_cmd_buffer(frame_index);
		const gfx_id	cmd_post		 = _pass_post_combiner.get_cmd_buffer(frame_index);

		upload(frame_index);

		_pass_pre_depth.render({
			.frame_index   = frame_index,
			.size		   = resolution,
			.global_layout = layout_global,
			.global_group  = bind_group_global,
		});

		//static_vector<std::function<void()>, 2> tasks;
		//tasks.push_back([&] {
		//
		//});
		//tasks.push_back([&] {
		//	
		//});

			_pass_opaque.render({
			.frame_index   = frame_index,
			.size		   = resolution,
			.global_layout = layout_global,
			.global_group  = bind_group_global,
		});

		_pass_lighting.render({
			.frame_index   = frame_index,
			.size		   = resolution,
			.global_layout = layout_global,
			.global_group  = bind_group_global,
		});

		//std::for_each(std::execution::par, tasks.begin(), tasks.end(), [](auto&& task) { task(); });

		if (prev_copy != next_copy)
			backend->queue_wait(queue_gfx, &sem_copy, &next_copy, 1);

		// submit depth
		backend->submit_commands(queue_gfx, &cmd_depth, 1);
		backend->queue_signal(queue_gfx, &sem_depth, &sem_depth_val, 1);

		// submit opaque, wait for depth
		backend->queue_wait(queue_gfx, &sem_depth, &sem_depth_val, 1);
		backend->submit_commands(queue_gfx, &cmd_opaque, 1);
		backend->queue_signal(queue_gfx, &sem_opaque, &sem_opaque_val, 1);

		// submit lighting, wait opaque
		backend->queue_wait(queue_gfx, &sem_opaque, &sem_opaque_val, 1);
		backend->submit_commands(queue_gfx, &cmd_lighting, 1);
		backend->queue_signal(queue_gfx, &sem_lighting, &sem_lighting_val, 1);
	}

	void world_renderer::resize(const vector2ui16& size)
	{
		_base_size			 = size;
		gfx_backend* backend = gfx_backend::get();

		// depth prepass
		_pass_pre_depth.resize(size);

		// gbuffer
		static_vector<gfx_id, BACK_BUFFER_COUNT> depths;
		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
			depths.push_back(_pass_pre_depth.get_depth_texture(i));

		_pass_opaque.resize(size, depths.data());

		// lighting
		static_vector<gfx_id, BACK_BUFFER_COUNT * 4> gbuffer_textures;
		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			gbuffer_textures.push_back(_pass_opaque.get_color_texture(i, 0));
			gbuffer_textures.push_back(_pass_opaque.get_color_texture(i, 1));
			gbuffer_textures.push_back(_pass_opaque.get_color_texture(i, 2));
			gbuffer_textures.push_back(_pass_opaque.get_color_texture(i, 3));
		}
		_pass_lighting.resize(size, gbuffer_textures.data(), depths.data());
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

	void world_renderer::upload(uint8 frame_index)
	{
		gfx_backend*	   backend = gfx_backend::get();
		per_frame_data&	   pfd	   = _pfd[frame_index];
		world_render_data& wrd	   = _render_data;

		const gfx_id queue_gfx = backend->get_queue_gfx();
		const gfx_id cmd	   = pfd.cmd_upload;

		backend->reset_command_buffer(cmd);

		static_vector<barrier, 20> barriers;

		barriers.push_back({
			.resource	= pfd.bones_buffer.get_hw_gpu(),
			.flags		= barrier_flags::baf_is_resource,
			.from_state = resource_state::non_ps_resource,
			.to_state	= resource_state::copy_dest,
		});

		barriers.push_back({
			.resource	= pfd.entity_buffer.get_hw_gpu(),
			.flags		= barrier_flags::baf_is_resource,
			.from_state = resource_state::non_ps_resource,
			.to_state	= resource_state::copy_dest,
		});

		barriers.push_back({
			.resource	= pfd.dir_lights_buffer.get_hw_gpu(),
			.flags		= barrier_flags::baf_is_resource,
			.from_state = resource_state::ps_resource,
			.to_state	= resource_state::copy_dest,
		});

		barriers.push_back({
			.resource	= pfd.spot_lights_buffer.get_hw_gpu(),
			.flags		= barrier_flags::baf_is_resource,
			.from_state = resource_state::ps_resource,
			.to_state	= resource_state::copy_dest,
		});

		barriers.push_back({
			.resource	= pfd.point_lights_buffer.get_hw_gpu(),
			.flags		= barrier_flags::baf_is_resource,
			.from_state = resource_state::ps_resource,
			.to_state	= resource_state::copy_dest,
		});

		backend->cmd_barrier(cmd, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

		if (!wrd.entities.empty())
			pfd.entity_buffer.copy_region(cmd, 0, wrd.entities.size() * sizeof(gpu_entity));

		if (!wrd.bones.empty())
			pfd.bones_buffer.copy_region(cmd, 0, wrd.bones.size() * sizeof(gpu_bone));

		if (!wrd.dir_lights.empty())
			pfd.dir_lights_buffer.copy_region(cmd, 0, wrd.dir_lights.size() * sizeof(gpu_dir_light));

		if (!wrd.point_lights.empty())
			pfd.point_lights_buffer.copy_region(cmd, 0, wrd.point_lights.size() * sizeof(gpu_point_light));

		if (!wrd.spot_lights.empty())
			pfd.spot_lights_buffer.copy_region(cmd, 0, wrd.spot_lights.size() * sizeof(gpu_spot_light));

		barriers.resize(0);

		barriers.push_back({
			.resource	= pfd.bones_buffer.get_hw_gpu(),
			.flags		= barrier_flags::baf_is_resource,
			.from_state = resource_state::copy_dest,
			.to_state	= resource_state::non_ps_resource,
		});

		barriers.push_back({
			.resource	= pfd.entity_buffer.get_hw_gpu(),
			.flags		= barrier_flags::baf_is_resource,
			.from_state = resource_state::copy_dest,
			.to_state	= resource_state::non_ps_resource,
		});

		barriers.push_back({
			.resource	= pfd.dir_lights_buffer.get_hw_gpu(),
			.flags		= barrier_flags::baf_is_resource,
			.from_state = resource_state::copy_dest,
			.to_state	= resource_state::ps_resource,
		});

		barriers.push_back({
			.resource	= pfd.spot_lights_buffer.get_hw_gpu(),
			.flags		= barrier_flags::baf_is_resource,
			.from_state = resource_state::copy_dest,
			.to_state	= resource_state::ps_resource,
		});

		barriers.push_back({
			.resource	= pfd.point_lights_buffer.get_hw_gpu(),
			.flags		= barrier_flags::baf_is_resource,
			.from_state = resource_state::copy_dest,
			.to_state	= resource_state::ps_resource,
		});

		backend->cmd_barrier(cmd, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});
		backend->close_command_buffer(cmd);
		backend->submit_commands(queue_gfx, &cmd, 1);
	}

	void world_renderer::collect_model_instances()
	{
		const uint32	   mesh_instances_peak = _proxy_manager.get_peak_mesh_instances();
		auto&			   mesh_instances	   = *_proxy_manager.get_mesh_instances();
		auto&			   entities			   = *_proxy_manager.get_entities();
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

			const uint32				  entity_index = create_gpu_entity({.model = proxy_entity.model, .normal = proxy_entity.normal, .position = proxy_entity.position});
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

	void world_renderer::collect_lights()
	{
		const uint32 points_peak  = _proxy_manager.get_peak_point_lights();
		const uint32 points_count = _proxy_manager.get_count_point_lights();
		const uint32 spots_peak	  = _proxy_manager.get_peak_spot_lights();
		const uint32 spots_count  = _proxy_manager.get_count_spot_lights();
		const uint32 dirs_peak	  = _proxy_manager.get_peak_dir_lights();
		const uint32 dirs_count	  = _proxy_manager.get_count_dir_lights();
		auto&		 points		  = *_proxy_manager.get_point_lights();
		auto&		 spots		  = *_proxy_manager.get_spot_lights();
		auto&		 dirs		  = *_proxy_manager.get_dir_lights();
		auto&		 entities	  = *_proxy_manager.get_entities();

		world_render_data& wrd = _render_data;

		for (uint32 i = 0; i < dirs_peak; i++)
		{
			const render_proxy_dir_light& light = dirs.get(i);
			if (light.status != render_proxy_status::rps_active)
				continue;

			const render_proxy_entity& proxy_entity = entities.get(light.entity);
			SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);
			if (proxy_entity.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
				continue;

			wrd.dir_lights.push_back({
				.color_entity_index = vector4(light.base_color.x, light.base_color.y, light.base_color.z, static_cast<float>(light.entity)),
			});
		}

		for (uint32 i = 0; i < points_peak; i++)
		{
			const render_proxy_point_light& light = points.get(i);
			if (light.status != render_proxy_status::rps_active)
				continue;

			const render_proxy_entity& proxy_entity = entities.get(light.entity);
			SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);
			if (proxy_entity.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
				continue;

			constexpr float energy_thresh = LIGHT_CULLING_ENERGY_THRESHOLD;
			const float		radius		  = math::sqrt(light.intensity / (4.0f * M_PI * energy_thresh));

			const frustum_result res = frustum::test(_main_camera_view.view_frustum, proxy_entity.position, radius);
			if (res == frustum_result::outside)
				continue;

			wrd.point_lights.push_back({
				.color_entity_index = vector4(light.base_color.x, light.base_color.y, light.base_color.z, static_cast<float>(light.entity)),
				.intensity_range	= vector4(light.intensity, light.range, 0.0f, 0.0f),
			});
		}

		for (uint32 i = 0; i < spots_peak; i++)
		{
			const render_proxy_spot_light& light = spots.get(i);
			if (light.status != render_proxy_status::rps_active)
				continue;

			const render_proxy_entity& proxy_entity = entities.get(light.entity);
			SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);
			if (proxy_entity.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
				continue;

			wrd.spot_lights.push_back({
				.color_entity_index			 = vector4(light.base_color.x, light.base_color.y, light.base_color.z, static_cast<float>(light.entity)),
				.intensity_range_inner_outer = vector4(light.intensity, light.range, light.inner_cone, light.outer_cone),
			});
		}
	}

}
