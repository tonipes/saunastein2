// Copyright (c) 2025 Inan Evin

#include "renderable_collector.hpp"
#include "gfx/proxy/render_proxy_entity.hpp"
#include "gfx/proxy/render_proxy_traits.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "math/math.hpp"
#include "math/math_common.hpp"

namespace SFG
{
	void renderable_collector::reset()
	{
		_renderables.resize(0);
		_entities.resize(0);
		_point_lights.resize(0);
		_dir_lights.resize(0);
		_spot_lights.resize(0);
		_bones.resize(0);
		_view = {};
	}

	void renderable_collector::generate_view_from_cam(const render_proxy_entity& entity, const render_proxy_camera& cam, const vector2ui16& resolution)
	{
		_view = {
			.view_matrix = matrix4x4::view(entity.rotation, entity.position),
			.proj_matrix = matrix4x4::perspective_reverse_z(cam.fov_degrees, static_cast<float>(resolution.x) / static_cast<float>(resolution.y), cam.near_plane, cam.far_plane),
		};
		_view.view_proj_matrix	   = _view.proj_matrix * _view.view_matrix;
		_view.inv_view_proj_matrix = _view.view_proj_matrix.inverse();
		_view.view_frustum		   = frustum::extract(_view.view_proj_matrix);
	}

	void renderable_collector::collect_entities(proxy_manager& pm)
	{
		auto&		 entities	   = *pm.get_entities();
		const uint32 entities_peak = pm.get_peak_entities();

		for (uint32 i = 0; i < entities_peak; i++)
		{
			render_proxy_entity& e = entities.get(i);
			if (e.status != render_proxy_status::rps_active)
				continue;

			if (e.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
			{
				e._current_buffer_index = MAX_GPU_ENTITIES;
				continue;
			}

			const vector3 forward = e.rotation.get_forward();

			e._current_buffer_index = static_cast<uint32>(_entities.size());
			_entities.push_back({
				.model	  = e.model.to_matrix4x4(),
				.normal	  = e.normal.to_matrix4x4(),
				.position = vector4(e.position.x, e.position.y, e.position.z, 0),
				.rotation = vector4(e.rotation.x, e.rotation.y, e.rotation.z, e.rotation.w),
				.forward  = vector4(forward.x, forward.y, forward.z, 0.0f),
			});
		}
	}

	void renderable_collector::collect_lights(proxy_manager& pm)
	{
		const uint32 points_peak  = pm.get_peak_point_lights();
		const uint32 points_count = pm.get_count_point_lights();
		const uint32 spots_peak	  = pm.get_peak_spot_lights();
		const uint32 spots_count  = pm.get_count_spot_lights();
		const uint32 dirs_peak	  = pm.get_peak_dir_lights();
		const uint32 dirs_count	  = pm.get_count_dir_lights();
		auto&		 points		  = *pm.get_point_lights();
		auto&		 spots		  = *pm.get_spot_lights();
		auto&		 dirs		  = *pm.get_dir_lights();
		auto&		 entities	  = *pm.get_entities();

		for (uint32 i = 0; i < points_peak; i++)
		{
			const render_proxy_point_light& light = points.get(i);
			if (light.status != render_proxy_status::rps_active)
				continue;

			const render_proxy_entity& proxy_entity = entities.get(light.entity);
			SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);
			if (proxy_entity._current_buffer_index == MAX_GPU_ENTITIES)
				continue;

			constexpr float energy_thresh = LIGHT_CULLING_ENERGY_THRESHOLD;
			const float		radius		  = math::sqrt(light.intensity / (4.0f * MATH_PI * energy_thresh));

			const frustum_result res = frustum::test(_view.view_frustum, proxy_entity.position, radius);
			if (res == frustum_result::outside)
				continue;

			_point_lights.push_back({
				.color_entity_index = vector4(light.base_color.x, light.base_color.y, light.base_color.z, static_cast<float>(proxy_entity._current_buffer_index)),
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
			if (proxy_entity._current_buffer_index == MAX_GPU_ENTITIES)
				continue;

			constexpr float energy_thresh = LIGHT_CULLING_ENERGY_THRESHOLD;
			const float		radius		  = math::sqrt(light.intensity / (4.0f * MATH_PI * energy_thresh));

			const frustum_result res = frustum::test(_view.view_frustum, proxy_entity.position, radius);
			if (res == frustum_result::outside)
				continue;

			_spot_lights.push_back({
				.color_entity_index			 = vector4(light.base_color.x, light.base_color.y, light.base_color.z, static_cast<float>(proxy_entity._current_buffer_index)),
				.intensity_range_inner_outer = vector4(light.intensity, light.range, math::cos(light.inner_cone), math::cos(light.outer_cone)),
			});
		}

		for (uint32 i = 0; i < dirs_peak; i++)
		{
			const render_proxy_dir_light& light = dirs.get(i);
			if (light.status != render_proxy_status::rps_active)
				continue;

			const render_proxy_entity& proxy_entity = entities.get(light.entity);
			SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);

			if (proxy_entity._current_buffer_index == MAX_GPU_ENTITIES)
				continue;

			_dir_lights.push_back({
				.color_entity_index = vector4(light.base_color.x, light.base_color.y, light.base_color.z, static_cast<float>(proxy_entity._current_buffer_index)),
				.intensity			= vector4(light.intensity, 0.0f, 0.0f, 0.0f),
			});
		}
	}
	void renderable_collector::collect_model_instances(proxy_manager& pm)
	{
		const uint32	   mesh_instances_peak = pm.get_peak_mesh_instances();
		auto&			   mesh_instances	   = *pm.get_mesh_instances();
		auto&			   entities			   = *pm.get_entities();
		chunk_allocator32& aux				   = pm.get_aux();

		for (uint32 i = 0; i < mesh_instances_peak; i++)
		{
			const render_proxy_mesh_instance& mesh_instance = mesh_instances.get(i);
			if (mesh_instance.status != render_proxy_status::rps_active)
				continue;

			const render_proxy_entity& proxy_entity = entities.get(mesh_instance.entity);
			SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);

			const render_proxy_mesh& proxy_mesh = pm.get_mesh(mesh_instance.mesh);
			if (proxy_mesh.primitives.size == 0)
				continue;

			aabb frustum_aabb = proxy_mesh.local_aabb;

			const frustum_result res = frustum::test(_view.view_frustum, proxy_mesh.local_aabb, proxy_entity.model.to_linear3x3(), proxy_entity.position);
			if (res == frustum_result::outside)
				continue;

			const render_proxy_model& proxy_model = pm.get_model(mesh_instance.model);

			const render_proxy_primitive* primitives = aux.get<render_proxy_primitive>(proxy_mesh.primitives);
			const uint16*				  materials	 = aux.get<uint16>(proxy_model.materials);

			for (uint32 i = 0; i < proxy_mesh.primitive_count; i++)
			{
				const render_proxy_primitive& prim = primitives[i];

				const uint16 mat = materials[prim.material_index];

				_renderables.push_back({
					.vertex_buffer = const_cast<buffer*>(&proxy_mesh.vertex_buffer),
					.index_buffer  = const_cast<buffer*>(&proxy_mesh.index_buffer),
					.vertex_start  = prim.vertex_start,
					.index_start   = prim.index_start,
					.index_count   = prim.index_count,
					.gpu_entity	   = proxy_entity._current_buffer_index,
					.material	   = mat,
					.is_skinned	   = proxy_mesh.is_skinned,
				});
			}
		}
	}
}
