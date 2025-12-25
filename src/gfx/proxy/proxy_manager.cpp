/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "proxy_manager.hpp"
#include "resources/common_resources.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "gfx/event_stream/render_events_entity.hpp"
#include "gfx/event_stream/render_events_trait.hpp"
#include "gfx/texture_queue.hpp"
#include "gfx/buffer_queue.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "common/system_info.hpp"
#include "data/istream.hpp"
#include "math/math.hpp"
#include "gui/vekt.hpp"
#include "platform/time.hpp"
#include <tracy/Tracy.hpp>

namespace SFG
{

	proxy_manager::proxy_manager(buffer_queue& b, texture_queue& t) : _buffer_queue(b), _texture_queue(t)
	{
		_textures		   = new textures_type();
		_samplers		   = new samplers_type();
		_materials		   = new materials_type();
		_material_runtimes = new material_runtimes_type();
		_shaders		   = new shaders_type();
		_meshes			   = new meshes_type();
		_skins			   = new skins_type();
		_models			   = new models_type();
		_entities		   = new entity_type();
		_mesh_instances	   = new mesh_instances_type();
		_cameras		   = new cameras_type();
		_ambients		   = new ambients_type();
		_point_lights	   = new point_lights_type();
		_spot_lights	   = new spot_lights_type();
		_dir_lights		   = new dir_lights_type();
		_canvases		   = new canvas_type();
	}

	proxy_manager::~proxy_manager()
	{
		delete _textures;
		delete _samplers;
		delete _materials;
		delete _material_runtimes;
		delete _shaders;
		delete _meshes;
		delete _skins;
		delete _models;
		delete _entities;
		delete _mesh_instances;
		delete _cameras;
		delete _ambients;
		delete _point_lights;
		delete _spot_lights;
		delete _dir_lights;
		delete _canvases;
	}

	void proxy_manager::reset()
	{
		_textures->reset();
		_samplers->reset();
		_materials->reset();
		_material_runtimes->reset();
		_shaders->reset();
		_meshes->reset();
		_skins->reset();
		_models->reset();
		_entities->reset();
		_mesh_instances->reset();
		_cameras->reset();
		_ambients->reset();
		_point_lights->reset();
		_spot_lights->reset();
		_dir_lights->reset();
		_canvases->reset();
	}

	void proxy_manager::init()
	{
		for (uint8 i = 0; i < BACK_BUFFER_COUNT + 1; i++)
		{
			_destroy_bucket[i].list.reserve(1000);
		}

		_aux_memory.init(1024 * 1024 * 2);
	}

	void proxy_manager::uninit()
	{
		auto& textures	= *_textures;
		auto& samplers	= *_samplers;
		auto& materials = *_materials;
		auto& shaders	= *_shaders;
		auto& meshes	= *_meshes;

		for (auto& res : *_textures)
		{
			if (res.status == 0)
				continue;

			destroy_texture(res);
		}

		for (auto& res : samplers)
		{
			if (res.status == 0)
				continue;

			destroy_sampler(res);
		}

		for (auto& res : shaders)
		{
			if (res.status == 0)
				continue;

			destroy_shader(res);
		}

		for (auto& res : materials)
		{
			if (res.status == 0)
				continue;

			destroy_material(res);
		}

		for (auto& res : meshes)
		{
			if (res.status == 0)
				continue;

			destroy_mesh(res);
		}

		flush_destroys(true);

		reset();

		_aux_memory.uninit();
	}

	void proxy_manager::fetch_render_events(render_event_stream& stream, uint8 frame_index)
	{
		ZoneScoped;

		istream in;
		stream.open_into(in);

		if (in.get_size() == 0)
			return;

		render_event_header header = {};

		while (!in.is_eof())
		{
			header.deserialize(in);
			process_event(header, in, frame_index);
		}
	}

	void proxy_manager::flush_destroys(bool force)
	{
		gfx_backend* backend = gfx_backend::get();

		const uint64 frame = frame_info::get_render_frame();
		if (frame < BACK_BUFFER_COUNT + 1)
			return;

		const uint8 mod			= BACK_BUFFER_COUNT + 1;
		const uint8 safe_bucket = (frame - BACK_BUFFER_COUNT) % mod;
		destroy_target_bucket(safe_bucket);
		if (force)
		{
			for (uint32 i = 0; i < mod; i++)
				destroy_target_bucket(i);
		}
	}

	gfx_id proxy_manager::get_shader_variant(resource_id shader_handle, uint32 flags)
	{
		const render_proxy_shader&		   proxy_shader	  = get_shader(shader_handle);
		const uint32					   variants_count = proxy_shader.variant_count;
		const render_proxy_shader_variant* var			  = _aux_memory.get<const render_proxy_shader_variant>(proxy_shader.variants);

		for (uint32 i = 0; i < variants_count; i++)
		{
			const render_proxy_shader_variant& v = var[i];
			if (v.variant_flags.is_all_set(flags))
			{
				return v.hw;
			}
		}

		return NULL_GFX_ID;
	}

	void proxy_manager::process_event(const render_event_header& header, istream& stream, uint8 frame_index)
	{
		gfx_backend* backend = gfx_backend::get();

		const world_id			index = header.index;
		const render_event_type type  = header.event_type;

		if (type == render_event_type::create_canvas)
		{
			render_event_canvas ev = {};
			ev.deserialize(stream);

			render_proxy_canvas& proxy = get_canvas(index);
			proxy.entity			   = ev.entity_index;
			proxy.status			   = render_proxy_status::rps_active;
			proxy.is_3d				   = ev.is_3d;

			for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
			{
				proxy.vertex_buffers[i].create(
					{
						.size		= ev.vertex_size,
						.flags		= resource_flags::rf_cpu_visible,
						.debug_name = "canvas_vertex",
					},
					{
						.size		= ev.vertex_size,
						.flags		= resource_flags::rf_vertex_buffer | resource_flags::rf_gpu_only,
						.debug_name = "canvas_vertex_gpu",
					});

				proxy.index_buffers[i].create(
					{
						.size		= ev.index_size,
						.flags		= resource_flags::rf_cpu_visible,
						.debug_name = "canvas_index",
					},
					{
						.size		= ev.index_size,
						.flags		= resource_flags::rf_index_buffer | resource_flags::rf_gpu_only,
						.debug_name = "canvas_index_index",
					});
			}

			proxy._draw_calls.reserve(1024);
		}

		else if (type == render_event_type::destroy_canvas)
		{
			render_proxy_canvas& proxy = get_canvas(index);
			destroy_canvas(proxy);
		}
		else if (type == render_event_type::canvas_reset_draws)
		{
			render_proxy_canvas& proxy = get_canvas(index);
			proxy._draw_calls.resize(0);
			proxy._max_vertex_offset = 0;
			proxy._max_index_offset	 = 0;
		}
		else if (type == render_event_type::canvas_update)
		{
			render_event_canvas ev = {};
			ev.deserialize(stream);

			render_proxy_canvas& proxy = get_canvas(index);
			proxy.is_3d				   = ev.is_3d;
		}
		else if (type == render_event_type::canvas_add_draw)
		{
			render_event_canvas_add_draw ev = {};
			ev.deserialize(stream);

			render_proxy_canvas& proxy = get_canvas(index);
			proxy._draw_calls.push_back({
				.clip		  = ev.clip,
				.start_index  = ev.start_index,
				.start_vertex = ev.start_vertex,
				.index_count  = ev.index_count,
				.mat_id		  = ev.material_handle,
				.atlas_id	  = ev.atlas_handle,
				.atlas_exists = ev.atlas_exists,
			});

			proxy.vertex_buffers[frame_index].buffer_data(proxy._max_vertex_offset, ev.vertex_data, ev.vertex_data_size);
			proxy.index_buffers[frame_index].buffer_data(proxy._max_index_offset, ev.index_data, ev.index_data_size);
			proxy._max_vertex_offset += ev.vertex_data_size;
			proxy._max_index_offset += ev.index_data_size;

			_peak_canvases = math::max(_peak_canvases, index);
		}
		else if (type == render_event_type::remove_entity)
		{
			render_proxy_entity& proxy = get_entity(index);
			proxy.status			   = render_proxy_status::rps_inactive;
		}
		else if (type == render_event_type::update_entity_transform)
		{
			render_event_entity_transform ev = {};
			ev.deserialize(stream);
			render_proxy_entity& proxy = get_entity(index);
			proxy.status			   = render_proxy_status::rps_active;
			proxy.handle			   = index;
			proxy.model				   = ev.abs_model;
			proxy.normal			   = ev.abs_model.to_linear3x3().inversed().transposed();
			proxy.position			   = ev.position;
			proxy.rotation			   = ev.rotation;

			_peak_entities = math::max(_peak_entities, index);
		}
		else if (type == render_event_type::update_entity_visibility)
		{
			render_event_entity_visibility ev = {};
			ev.deserialize(stream);
			render_proxy_entity& proxy = get_entity(index);
			proxy.status			   = render_proxy_status::rps_active;
			proxy.handle			   = index;
			proxy.flags.set(render_proxy_entity_flags::render_proxy_entity_invisible, !ev.visible);
		}
		else if (type == render_event_type::update_ambient)
		{
			render_event_ambient ev = {};
			ev.deserialize(stream);
			_ambient_exists = 1;

			render_proxy_ambient& proxy = get_ambient();
			proxy.status				= render_proxy_status::rps_active;
			proxy.entity				= ev.entity_index;
			proxy.base_color			= ev.base_color;
		}
		else if (type == render_event_type::update_spot_light)
		{
			render_event_spot_light ev = {};
			ev.deserialize(stream);
			_peak_spot_lights = math::max(_peak_spot_lights, index);

			render_proxy_spot_light& proxy = _spot_lights->get(index);

			if (proxy.status != render_proxy_status::rps_active)
				_count_spot_lights++;

			const vector2ui16 pre_res = proxy.shadow_res;
			proxy.status			  = render_proxy_status::rps_active;
			proxy.entity			  = ev.entity_index;
			proxy.base_color		  = ev.base_color;
			proxy.intensity			  = ev.intensity;
			proxy.near_plane		  = ev.near_plane;
			proxy.inner_cone		  = ev.inner_cone;
			proxy.outer_cone		  = ev.outer_cone;
			proxy.cast_shadows		  = ev.cast_shadows;
			proxy.shadow_res		  = ev.shadow_resolution;

			auto create_shadow_texture = [&]() {
				// create textures
				vector<view_desc> views;

				views.push_back({
					.type			= view_type::depth_stencil,
					.base_arr_level = 0,
					.level_count	= 1,
				});

				views.push_back({
					.type			= view_type::sampled,
					.base_arr_level = 0,
					.level_count	= 1,
				});

				for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					proxy.shadow_texture_hw[i] = backend->create_texture({
						.texture_format		  = render_target_definitions::get_format_depth_default_read(),
						.depth_stencil_format = render_target_definitions::get_format_shadows(),
						.size				  = proxy.shadow_res,
						.flags				  = texture_flags::tf_is_2d | texture_flags::tf_sampled | texture_flags::tf_depth_texture | texture_flags::tf_typeless,
						.views				  = views,
						.array_length		  = 1,
						.clear_values		  = {1.0f, 0.0f, 0.0f, 0.0f},
						.debug_name			  = "spot_light_shadow",
					});

					proxy.shadow_texture_gpu_index[i] = backend->get_texture_gpu_index(proxy.shadow_texture_hw[i], views.size() - 1);
				}
			};

			auto kill_shadow_texture = [&]() {
				const uint8 safe_bucket = frame_info::get_render_frame() % (BACK_BUFFER_COUNT + 1);
				for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					add_to_destroy_bucket({.id = proxy.shadow_texture_hw[i], .type = destroy_data_type::texture}, safe_bucket);
					proxy.shadow_texture_hw[i] = NULL_GFX_ID;
				}
			};

			if (proxy.cast_shadows == 0)
			{
				if (proxy.shadow_texture_hw[0] != NULL_GFX_ID)
					kill_shadow_texture();
			}
			else
			{
				if (proxy.shadow_texture_hw[0] == NULL_GFX_ID)
				{
					create_shadow_texture();
				}
				else if (proxy.shadow_res != pre_res)
				{
					kill_shadow_texture();
					create_shadow_texture();
				}
			}
		}
		else if (type == render_event_type::update_point_light)
		{
			render_event_point_light ev = {};
			ev.deserialize(stream);
			_peak_point_lights = math::max(_peak_point_lights, index);

			render_proxy_point_light& proxy = _point_lights->get(index);

			if (proxy.status != render_proxy_status::rps_active)
				_count_point_lights++;

			const vector2ui16 pre_res = proxy.shadow_res;
			proxy.status			  = render_proxy_status::rps_active;
			proxy.entity			  = ev.entity_index;
			proxy.base_color		  = ev.base_color;
			proxy.range				  = ev.range;
			proxy.intensity			  = ev.intensity;
			proxy.near_plane		  = ev.near_plane;
			proxy.cast_shadows		  = ev.cast_shadows;
			proxy.shadow_res		  = ev.shadow_resolution;

			auto create_shadow_texture = [&]() {
				// create textures
				vector<view_desc> views;

				for (uint8 i = 0; i < 6; i++)
				{
					views.push_back({
						.type			= view_type::depth_stencil,
						.base_arr_level = i,
						.level_count	= 1,
					});
				}
				views.push_back({
					.type			= view_type::sampled,
					.base_arr_level = 0,
					.level_count	= 1,
					.is_cubemap		= 1,
				});

				for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					proxy.shadow_texture_hw[i] = backend->create_texture({
						.texture_format		  = render_target_definitions::get_format_depth_default_read(),
						.depth_stencil_format = render_target_definitions::get_format_shadows(),
						.size				  = proxy.shadow_res,
						.flags				  = texture_flags::tf_is_2d | texture_flags::tf_sampled | texture_flags::tf_depth_texture | texture_flags::tf_typeless,
						.views				  = views,
						.array_length		  = 6,
						.clear_values		  = {1.0f, 0.0f, 0.0f, 0.0f},
						.debug_name			  = "point_light_shadow",
					});

					proxy.shadow_texture_gpu_index[i] = backend->get_texture_gpu_index(proxy.shadow_texture_hw[i], views.size() - 1);
				}
			};

			auto kill_shadow_texture = [&]() {
				const uint8 safe_bucket = frame_info::get_render_frame() % (BACK_BUFFER_COUNT + 1);
				for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					add_to_destroy_bucket({.id = proxy.shadow_texture_hw[i], .type = destroy_data_type::texture}, safe_bucket);
					proxy.shadow_texture_hw[i] = NULL_GFX_ID;
				}
			};

			if (proxy.cast_shadows == 0)
			{
				if (proxy.shadow_texture_hw[0] != NULL_GFX_ID)
					kill_shadow_texture();
			}
			else
			{
				if (proxy.shadow_texture_hw[0] == NULL_GFX_ID)
				{
					create_shadow_texture();
				}
				else if (proxy.shadow_res != pre_res)
				{
					kill_shadow_texture();
					create_shadow_texture();
				}
			}
		}
		else if (type == render_event_type::update_dir_light)
		{
			render_event_dir_light ev = {};
			ev.deserialize(stream);
			_peak_dir_lights = math::max(_peak_dir_lights, index);

			render_proxy_dir_light& proxy = _dir_lights->get(index);

			if (proxy.status != render_proxy_status::rps_active)
				_count_dir_lights++;

			const vector2ui16 pre_res = proxy.shadow_res;

			proxy.status		 = render_proxy_status::rps_active;
			proxy.entity		 = ev.entity_index;
			proxy.base_color	 = ev.base_color;
			proxy.intensity		 = ev.intensity;
			proxy.cast_shadows	 = ev.cast_shadows;
			proxy.shadow_res	 = ev.shadow_resolution;
			proxy.cascade_levels = ev.max_cascades;

			auto create_shadow_texture = [&]() {
				// create textures
				vector<view_desc> views;

				const uint8 cascade_size = ev.max_cascades;
				for (uint8 i = 0; i < cascade_size; i++)
				{
					views.push_back({
						.type			= view_type::depth_stencil,
						.base_arr_level = i,
						.level_count	= 1,
					});
				}

				views.push_back({
					.type			= view_type::sampled,
					.base_arr_level = 0,
					.level_count	= cascade_size,
				});

				for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					proxy.shadow_texture_hw[i] = backend->create_texture({
						.texture_format		  = render_target_definitions::get_format_depth_default_read(),
						.depth_stencil_format = render_target_definitions::get_format_shadows(),
						.size				  = proxy.shadow_res,
						.flags				  = texture_flags::tf_is_2d | texture_flags::tf_sampled | texture_flags::tf_depth_texture | texture_flags::tf_typeless,
						.views				  = views,
						.array_length		  = cascade_size,
						.clear_values		  = {1.0f, 0.0f, 0.0f, 0.0f},
						.debug_name			  = "dir_light_shadow",
					});

					proxy.shadow_texture_gpu_index[i] = backend->get_texture_gpu_index(proxy.shadow_texture_hw[i], views.size() - 1);
				}
			};

			auto kill_shadow_texture = [&]() {
				const uint8 safe_bucket = frame_info::get_render_frame() % (BACK_BUFFER_COUNT + 1);
				for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					add_to_destroy_bucket({.id = proxy.shadow_texture_hw[i], .type = destroy_data_type::texture}, safe_bucket);
					proxy.shadow_texture_hw[i] = NULL_GFX_ID;
				}
			};

			if (proxy.cast_shadows == 0)
			{
				if (proxy.shadow_texture_hw[0] != NULL_GFX_ID)
					kill_shadow_texture();
			}
			else
			{
				if (proxy.shadow_texture_hw[0] == NULL_GFX_ID)
				{
					create_shadow_texture();
				}
				else if (proxy.shadow_res != pre_res)
				{
					kill_shadow_texture();
					create_shadow_texture();
				}
			}
		}
		else if (type == render_event_type::remove_ambient)
		{
			render_proxy_ambient& proxy = get_ambient();
			proxy.status				= render_proxy_status::rps_active;
			proxy						= {};
			_ambient_exists				= 0;
		}
		else if (type == render_event_type::remove_point_light)
		{
			render_proxy_point_light& proxy = _point_lights->get(index);

			if (proxy.status != render_proxy_status::rps_inactive)
				_count_point_lights--;

			if (proxy.shadow_texture_hw[0] != NULL_GFX_ID)
			{
				const uint8 safe_bucket = frame_info::get_render_frame() % (BACK_BUFFER_COUNT + 1);

				for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					add_to_destroy_bucket({.id = proxy.shadow_texture_hw[i], .type = destroy_data_type::texture}, safe_bucket);
					proxy.shadow_texture_hw[i] = NULL_GFX_ID;
				}
			}

			proxy.status = render_proxy_status::rps_active;
			proxy		 = {};
		}
		else if (type == render_event_type::remove_spot_light)
		{
			render_proxy_spot_light& proxy = _spot_lights->get(index);

			if (proxy.status != render_proxy_status::rps_inactive)
				_count_spot_lights--;

			if (proxy.shadow_texture_hw[0] != NULL_GFX_ID)
			{
				const uint8 safe_bucket = frame_info::get_render_frame() % (BACK_BUFFER_COUNT + 1);

				for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					add_to_destroy_bucket({.id = proxy.shadow_texture_hw[i], .type = destroy_data_type::texture}, safe_bucket);
					proxy.shadow_texture_hw[i] = NULL_GFX_ID;
				}
			}

			proxy.status = render_proxy_status::rps_active;
			proxy		 = {};
		}
		else if (type == render_event_type::remove_dir_light)
		{
			render_proxy_dir_light& proxy = _dir_lights->get(index);

			if (proxy.status != render_proxy_status::rps_inactive)
				_count_dir_lights--;

			if (proxy.shadow_texture_hw[0] != NULL_GFX_ID)
			{
				const uint8 safe_bucket = frame_info::get_render_frame() % (BACK_BUFFER_COUNT + 1);

				for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					add_to_destroy_bucket({.id = proxy.shadow_texture_hw[i], .type = destroy_data_type::texture}, safe_bucket);
					proxy.shadow_texture_hw[i] = NULL_GFX_ID;
				}
			}

			proxy.status = render_proxy_status::rps_active;
			proxy		 = {};
		}
		else if (type == render_event_type::update_mesh_instance)
		{
			render_event_mesh_instance ev = {};
			ev.deserialize(stream);
			_peak_mesh_instances = math::max(_peak_mesh_instances, index);

			render_proxy_mesh_instance& proxy = get_mesh_instance(index);

			proxy.status = render_proxy_status::rps_active;
			proxy.entity = ev.entity_index;
			proxy.mesh	 = ev.mesh;
			proxy.model	 = ev.model;
			proxy.skin	 = ev.skin;

			if (proxy.skin_entities.size != 0)
			{
				_aux_memory.free(proxy.skin_entities);
			}

			if (!ev.skin_node_entities.empty())
			{
				proxy.skin_entities_count = static_cast<uint16>(ev.skin_node_entities.size());
				proxy.skin_entities		  = _aux_memory.allocate<world_id>(proxy.skin_entities_count);
				world_id* ptr			  = _aux_memory.get<world_id>(proxy.skin_entities);
				for (uint16 i = 0; i < proxy.skin_entities_count; i++)
				{
					ptr[i] = ev.skin_node_entities[i];
				}
			}
		}
		else if (type == render_event_type::remove_mesh_instance)
		{
			render_proxy_mesh_instance& proxy = get_mesh_instance(index);
			proxy							  = {};
		}
		else if (type == render_event_type::update_camera)
		{
			render_event_camera ev = {};
			ev.deserialize(stream);

			render_proxy_camera& proxy = get_camera(index);

			proxy = {
				.entity		 = ev.entity_index,
				.near_plane	 = ev.near_plane,
				.far_plane	 = ev.far_plane,
				.fov_degrees = ev.fov_degrees,
			};

			const uint8 cascade_levels = static_cast<uint8>(ev.cascades.size());

			if (cascade_levels != 0)
			{
				chunk_handle32 cascades		= _aux_memory.allocate<float>(cascade_levels);
				float*		   cascades_ptr = _aux_memory.get<float>(cascades);
				for (uint8 i = 0; i < cascade_levels; i++)
					cascades_ptr[i] = ev.cascades[i];
				proxy.cascades		= cascades;
				proxy.cascade_count = cascade_levels;
			}
		}
		else if (type == render_event_type::set_main_camera)
		{
			render_proxy_camera& proxy = get_camera(index);
			_main_camera_trait		   = index;
		}
		else if (type == render_event_type::remove_camera)
		{
			render_proxy_camera& proxy = get_camera(index);
			if (index == _main_camera_trait)
				_main_camera_trait = NULL_WORLD_ID;

			if (proxy.cascades.size != 0)
				_aux_memory.free(proxy.cascades);

			proxy = {};
		}
		else if (type == render_event_type::create_texture)
		{
			render_event_texture ev = {};
			ev.deserialize(stream);

			render_proxy_texture& proxy = get_texture(index);
			proxy.status				= render_proxy_status::rps_active;
			proxy.handle				= index;

			proxy.hw = backend->create_texture({
				.texture_format = static_cast<format>(ev.format),
				.size			= ev.size,
				.flags			= texture_flags::tf_is_2d | texture_flags::tf_sampled,
				.views			= {{
							 .type			 = view_type::sampled,
							 .base_arr_level = 0,
							 .level_count	 = 1,
							 .base_mip_level = 0,
							 .mip_count		 = static_cast<uint8>(ev.buffers.size()),
				 }},
				.mip_levels		= static_cast<uint8>(ev.buffers.size()),
				.array_length	= 1,
				.samples		= 1,
#ifndef SFG_STRIP_DEBUG_NAMES
				.debug_name = ev.name.c_str(),
#endif
			});

			proxy.heap_index = backend->get_texture_gpu_index(proxy.hw, 0);

			proxy.intermediate = backend->create_resource({
				.size		= ev.intermediate_size,
				.flags		= resource_flags::rf_cpu_visible,
				.debug_name = "texture_intermediate",
			});

			_texture_queue.add_request(ev.buffers, proxy.hw, proxy.intermediate, 1, resource_state::resource_state_ps_resource);
		}
		else if (type == render_event_type::destroy_texture)
		{
			render_proxy_texture& proxy = get_texture(index);
			destroy_texture(proxy);
		}
		else if (type == render_event_type::create_sampler)
		{
			render_event_sampler ev = {};
			ev.deserialize(stream);
			render_proxy_sampler& proxy = get_sampler(index);
			proxy.status				= render_proxy_status::rps_active;
			proxy.handle				= index;
			proxy.hw					= backend->create_sampler(ev.desc);
			proxy.heap_index			= backend->get_sampler_gpu_index(proxy.hw);
		}
		else if (type == render_event_type::destroy_sampler)
		{
			render_proxy_sampler& proxy = get_sampler(index);
			destroy_sampler(proxy);
		}
		else if (type == render_event_type::create_shader)
		{
			render_event_shader ev = {};
			ev.deserialize(stream);
			render_proxy_shader& proxy = get_shader(index);
			proxy.status			   = render_proxy_status::rps_active;
			proxy.handle			   = index;

			const uint32 pso_count = static_cast<uint32>(ev.pso_variants.size());
			proxy.variants		   = _aux_memory.allocate<render_proxy_shader_variant>(pso_count);
			proxy.variant_count	   = pso_count;

			render_proxy_shader_variant* proxy_vars = _aux_memory.get<render_proxy_shader_variant>(proxy.variants);

			for (uint32 i = 0; i < pso_count; i++)
			{
				render_proxy_shader_variant& variant = proxy_vars[i];
				pso_variant&				 pso	 = ev.pso_variants[i];

				variant.variant_flags = pso.variant_flags;
				variant.hw			  = backend->create_shader(pso.desc, ev.compile_variants.at(pso.compile_variant).blobs, ev.layout);
			}

			for (compile_variant& cv : ev.compile_variants)
				cv.destroy();
			ev.compile_variants.clear();
		}
		else if (type == render_event_type::destroy_shader)
		{
			render_proxy_shader& proxy = get_shader(index);
			const chunk_handle32 vars  = proxy.variants;
			destroy_shader(proxy);
			_aux_memory.free(vars);
		}
		else if (type == render_event_type::reload_shader)
		{
			render_event_resource_reloaded ev = {};
			ev.deserialize(stream);
			for (uint32 i = 0; i < MAX_WORLD_MATERIALS; i++)
			{
				render_proxy_material&		   mat = _materials->get(i);
				render_proxy_material_runtime& rt  = get_material_runtime(i);
				if (mat.status != render_proxy_status::rps_active)
					continue;

				if (rt.shader_handle == ev.prev_id)
					rt.shader_handle = ev.new_id;
			}
		}
		else if (type == render_event_type::update_material_sampler)
		{
			render_event_update_material_sampler ev = {};
			ev.deserialize(stream);

			render_proxy_sampler& smp = get_sampler(ev.sampler);
			SFG_ASSERT(smp.status == render_proxy_status::rps_active);

			render_proxy_material& proxy = get_material(index);
			SFG_ASSERT(proxy.status == render_proxy_status::rps_active);

			render_proxy_material_runtime& runtime = get_material_runtime(index);
			runtime.gpu_index_sampler			   = backend->get_sampler_gpu_index(smp.hw);
		}
		else if (type == render_event_type::create_material)
		{
			render_event_material ev = {};
			ev.deserialize(stream);

			render_proxy_material&		   proxy   = get_material(index);
			render_proxy_material_runtime& runtime = get_material_runtime(index);
			proxy.status						   = render_proxy_status::rps_active;
			proxy.handle						   = index;
			runtime.flags						   = ev.flags;
			runtime.shader_handle				   = ev.shader_index;
			runtime.draw_priority				   = ev.priority;

			if ((ev.sampler.is_null() && !ev.textures.empty()) || (!ev.sampler.is_null() && ev.textures.empty()))
			{
				SFG_ASSERT(false);
			}

			for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
			{
				if (ev.data.size != 0)
				{
					proxy.buffers[i].create_staging_hw(
						{
							.size  = static_cast<uint32>(ev.data.size),
							.flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible,
#ifndef SFG_STRIP_DEBUG_NAMES
							.debug_name = ev.name.c_str(),
#endif
						},
						{
							.size  = static_cast<uint32>(ev.data.size),
							.flags = resource_flags::rf_constant_buffer | resource_flags::rf_gpu_only,
#ifndef SFG_STRIP_DEBUG_NAMES
							.debug_name = ev.name.c_str(),
#endif
						});

					proxy.buffers[i].buffer_data(0, ev.data.data, ev.data.size);
					runtime.gpu_index_buffers[i] = backend->get_resource_gpu_index(proxy.buffers[i].get_hw_gpu());

					_buffer_queue.add_request({
						.buffer	  = &proxy.buffers[i],
						.to_state = resource_state::resource_state_vertex_cbv,
					});
				}

				const uint32 texture_buffer_size = static_cast<uint32>(sizeof(gpu_index) * ev.textures.size());

				if (!ev.sampler.is_null())
				{
					runtime.gpu_index_sampler = backend->get_sampler_gpu_index(get_sampler(ev.sampler.index).hw);
				}

				if (texture_buffer_size != 0)
				{
					proxy.texture_buffers[i].create_staging_hw(
						{
							.size  = texture_buffer_size,
							.flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible,
#ifndef SFG_STRIP_DEBUG_NAMES
							.debug_name = ev.name.c_str(),
#endif
						},
						{
							.size  = texture_buffer_size,
							.flags = resource_flags::rf_constant_buffer | resource_flags::rf_gpu_only,
#ifndef SFG_STRIP_DEBUG_NAMES
							.debug_name = ev.name.c_str(),
#endif
						});

					runtime.gpu_index_texture_buffers[i] = backend->get_resource_gpu_index(proxy.texture_buffers[i].get_hw_gpu());

					size_t offset = 0;

					for (resource_handle txt_handle : ev.textures)
					{
						const render_proxy_texture& txt = get_texture(txt_handle.index);
						SFG_ASSERT(txt.status == render_proxy_status::rps_active);
						proxy.texture_buffers[i].buffer_data(offset, &txt.heap_index, sizeof(gpu_index));
						offset += sizeof(gpu_index);
					}

					_buffer_queue.add_request({
						.buffer	  = &proxy.texture_buffers[i],
						.to_state = resource_state::resource_state_vertex_cbv,
					});
				}
			}

			SFG_FREE(ev.data.data);
		}
		else if (type == render_event_type::update_material)
		{
			render_event_material ev = {};
			ev.deserialize(stream);

			render_proxy_material& proxy = get_material(index);

			for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
			{
				_buffer_queue.add_request(
					{
						.buffer	   = &proxy.buffers[i],
						.data	   = ev.data.data,
						.data_size = ev.data.size,
						.to_state  = resource_state::resource_state_vertex_cbv,
					},
					i);
			}
		}
		else if (type == render_event_type::destroy_material)
		{
			render_proxy_material& proxy = get_material(index);
			destroy_material(proxy);
		}
		else if (type == render_event_type::create_model)
		{
			render_proxy_model& proxy = get_model(index);
			render_event_model	ev	  = {};
			ev.deserialize(stream);

			proxy.material_count = static_cast<uint32>(ev.materials.size());
			proxy.mesh_count	 = static_cast<uint32>(ev.meshes.size());

			if (proxy.mesh_count != 0)
			{
				proxy.meshes		= _aux_memory.allocate<resource_id>(proxy.mesh_count);
				resource_id* meshes = _aux_memory.get<resource_id>(proxy.meshes);
				for (uint32 i = 0; i < proxy.mesh_count; i++)
					meshes[i] = ev.meshes[i];
			}

			if (proxy.material_count != 0)
			{
				proxy.materials	  = _aux_memory.allocate<resource_id>(proxy.material_count);
				resource_id* mats = _aux_memory.get<resource_id>(proxy.materials);
				for (uint32 i = 0; i < proxy.material_count; i++)
					mats[i] = ev.materials[i];
			}
		}
		else if (type == render_event_type::update_model_materials)
		{
			render_proxy_model&					proxy = get_model(index);
			render_event_model_update_materials ev	  = {};
			ev.deserialize(stream);

			const uint32 sz = static_cast<uint32>(ev.materials.size());

			if (sz != proxy.material_count)
			{
				if (proxy.materials.size != 0)
					_aux_memory.free(proxy.materials);
				proxy.materials = {};
			}

			proxy.material_count = sz;

			if (proxy.material_count != 0)
			{
				proxy.materials	  = _aux_memory.allocate<resource_id>(proxy.material_count);
				resource_id* mats = _aux_memory.get<resource_id>(proxy.materials);
				for (uint32 i = 0; i < proxy.material_count; i++)
					mats[i] = ev.materials[i];
			}
		}
		else if (type == render_event_type::destroy_model)
		{
			render_proxy_model& proxy = get_model(index);
			destroy_model(proxy);
		}
		else if (type == render_event_type::create_skin)
		{
			render_event_skin ev = {};
			ev.deserialize(stream);

			const uint32	   sz	 = static_cast<uint32>(ev.nodes.size());
			render_proxy_skin& proxy = get_skin(index);
			proxy.status			 = render_proxy_status::rps_active;
			proxy.nodes				 = _aux_memory.allocate<uint16>(sz);
			proxy.matrices			 = _aux_memory.allocate<matrix4x3>(sz);
			proxy.node_count		 = sz;

			uint16*	   nodes	= _aux_memory.get<uint16>(proxy.nodes);
			matrix4x3* matrices = _aux_memory.get<matrix4x3>(proxy.matrices);

			for (uint32 i = 0; i < sz; i++)
			{
				nodes[i]	= ev.nodes[i];
				matrices[i] = ev.matrices[i];
			}
		}
		else if (type == render_event_type::destroy_skin)
		{
			render_proxy_skin& proxy = get_skin(index);
			destroy_skin(proxy);
		}
		else if (type == render_event_type::create_mesh)
		{
			render_event_mesh ev = {};
			ev.deserialize(stream);
			render_proxy_mesh& proxy = get_mesh(index);
			proxy.status			 = render_proxy_status::rps_active;
			proxy.handle			 = index;
			proxy.local_aabb		 = ev.local_aabb;

			auto process = [&](auto& list, size_t vtx_type_size) {
				size_t vertex_size = 0;
				size_t index_size  = 0;

				for (const auto& p : list)
				{
					const size_t vertices = p.vertices.size();
					const size_t indices  = p.indices.size();
					vertex_size += vertices * vtx_type_size;
					index_size += indices * sizeof(primitive_index);
				}

				proxy.vertex_buffer.create_staging_hw(
					{
						.size  = static_cast<uint32>(vertex_size),
						.flags = resource_flags::rf_cpu_visible,
#ifndef SFG_STRIP_DEBUG_NAMES
						.debug_name = ev.name.c_str(),
#endif
					},
					{
						.size  = static_cast<uint32>(vertex_size),
						.flags = resource_flags::rf_vertex_buffer | resource_flags::rf_gpu_only,
#ifndef SFG_STRIP_DEBUG_NAMES
						.debug_name = ev.name.c_str(),
#endif
					});

				proxy.index_buffer.create_staging_hw(
					{
						.size  = static_cast<uint32>(index_size),
						.flags = resource_flags::rf_cpu_visible,
#ifndef SFG_STRIP_DEBUG_NAMES
						.debug_name = ev.name.c_str(),
#endif
					},
					{
						.size  = static_cast<uint32>(index_size),
						.flags = resource_flags::rf_index_buffer | resource_flags::rf_gpu_only,
#ifndef SFG_STRIP_DEBUG_NAMES
						.debug_name = ev.name.c_str(),
#endif
					});

				uint32 vtx_counter	 = 0;
				uint32 idx_counter	 = 0;
				size_t vertex_offset = 0;
				size_t index_offset	 = 0;

				const uint32 primitives_size	 = static_cast<uint32>(list.size());
				proxy.primitives				 = _aux_memory.allocate<render_proxy_primitive>(primitives_size);
				proxy.primitive_count			 = primitives_size;
				render_proxy_primitive* prim_ptr = _aux_memory.get<render_proxy_primitive>(proxy.primitives);

				for (uint32 i = 0; i < primitives_size; i++)
				{
					render_proxy_primitive& proxy_prim = prim_ptr[i];
					const auto&				p		   = list[i];
					const size_t			vtx_sz	   = p.vertices.size() * vtx_type_size;
					const size_t			idx_sz	   = p.indices.size() * sizeof(primitive_index);

					proxy_prim.material_index = p.material_index;
					proxy_prim.vertex_start	  = vtx_counter;
					proxy_prim.index_start	  = idx_counter;
					proxy_prim.index_count	  = static_cast<uint32>(p.indices.size());

					proxy.vertex_buffer.buffer_data(vertex_offset, p.vertices.data(), vtx_sz);
					proxy.index_buffer.buffer_data(index_offset, p.indices.data(), idx_sz);

					vertex_offset += vtx_sz;
					index_offset += idx_sz;
					vtx_counter += static_cast<uint32>(p.vertices.size());
					idx_counter += proxy_prim.index_count;
				}

				_buffer_queue.add_request({
					.buffer	  = &proxy.vertex_buffer,
					.to_state = resource_state::resource_state_vertex_cbv,
				});
				_buffer_queue.add_request({
					.buffer	  = &proxy.index_buffer,
					.to_state = resource_state::resource_state_index_buffer,
				});
			};

			if (!ev.primitives_static.empty())
			{
				proxy.is_skinned = 0;
				process(ev.primitives_static, sizeof(vertex_static));
			}
			else if (!ev.primitives_skinned.empty())
			{
				proxy.is_skinned = 1;
				process(ev.primitives_skinned, sizeof(vertex_skinned));
			}
			else
			{
				SFG_ASSERT(false);
			}
		}
		else if (type == render_event_type::destroy_mesh)
		{
			render_proxy_mesh& proxy = get_mesh(index);
			destroy_mesh(proxy);
		}
	}

	void proxy_manager::destroy_texture(render_proxy_texture& proxy)
	{
		const uint8 safe_bucket = frame_info::get_render_frame() % (BACK_BUFFER_COUNT + 1);
		add_to_destroy_bucket({.id = proxy.hw, .type = destroy_data_type::texture}, safe_bucket);
		add_to_destroy_bucket({.id = proxy.intermediate, .type = destroy_data_type::resource}, safe_bucket);
		proxy = {};
	}

	void proxy_manager::destroy_sampler(render_proxy_sampler& proxy)
	{
		const uint8 safe_bucket = frame_info::get_render_frame() % (BACK_BUFFER_COUNT + 1);
		add_to_destroy_bucket({.id = proxy.hw, .type = destroy_data_type::sampler}, safe_bucket);
		proxy = {};
	}

	void proxy_manager::destroy_shader(render_proxy_shader& proxy)
	{
		const uint8 safe_bucket = frame_info::get_render_frame() % (BACK_BUFFER_COUNT + 1);

		render_proxy_shader_variant* vars = _aux_memory.get<render_proxy_shader_variant>(proxy.variants);
		for (uint32 i = 0; i < proxy.variant_count; i++)
			add_to_destroy_bucket({.id = vars[i].hw, .type = destroy_data_type::shader}, safe_bucket);
		proxy = {};
	}

	void proxy_manager::destroy_mesh(render_proxy_mesh& proxy)
	{
		const uint8 safe_bucket = frame_info::get_render_frame() % (BACK_BUFFER_COUNT + 1);
		add_to_destroy_bucket({.id = proxy.vertex_buffer.get_hw_staging(), .type = destroy_data_type::resource}, safe_bucket);
		add_to_destroy_bucket({.id = proxy.vertex_buffer.get_hw_gpu(), .type = destroy_data_type::resource}, safe_bucket);
		add_to_destroy_bucket({.id = proxy.index_buffer.get_hw_staging(), .type = destroy_data_type::resource}, safe_bucket);
		add_to_destroy_bucket({.id = proxy.index_buffer.get_hw_gpu(), .type = destroy_data_type::resource}, safe_bucket);

		if (proxy.primitives.size != 0)
			_aux_memory.free(proxy.primitives);
		proxy = {};
	}

	void proxy_manager::destroy_model(render_proxy_model& proxy)
	{
		if (proxy.materials.size != 0)
			_aux_memory.free(proxy.materials);

		if (proxy.meshes.size != 0)
			_aux_memory.free(proxy.meshes);
		proxy = {};
	}

	void proxy_manager::destroy_canvas(render_proxy_canvas& proxy)
	{
		for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			const uint8 safe_bucket = (frame_info::get_render_frame() + i) % (BACK_BUFFER_COUNT + 1);
			add_to_destroy_bucket({.id = proxy.vertex_buffers[i].get_hw_staging(), .type = destroy_data_type::resource}, safe_bucket);
			add_to_destroy_bucket({.id = proxy.vertex_buffers[i].get_hw_gpu(), .type = destroy_data_type::resource}, safe_bucket);
			add_to_destroy_bucket({.id = proxy.index_buffers[i].get_hw_staging(), .type = destroy_data_type::resource}, safe_bucket);
			add_to_destroy_bucket({.id = proxy.index_buffers[i].get_hw_gpu(), .type = destroy_data_type::resource}, safe_bucket);
		}

		proxy = {};
	}

	void proxy_manager::destroy_skin(render_proxy_skin& proxy)
	{
		proxy = {};
	}

	void proxy_manager::destroy_material(render_proxy_material& proxy)
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			const uint8 safe_bucket = (frame_info::get_render_frame() + i) % (BACK_BUFFER_COUNT + 1);
			if (proxy.texture_buffers[i].get_hw_gpu() != NULL_GFX_ID)
			{
				add_to_destroy_bucket({.id = proxy.texture_buffers[i].get_hw_staging(), .type = destroy_data_type::resource}, safe_bucket);
				add_to_destroy_bucket({.id = proxy.texture_buffers[i].get_hw_gpu(), .type = destroy_data_type::resource}, safe_bucket);
			}

			if (proxy.buffers[i].get_hw_gpu() != NULL_GFX_ID)
			{
				add_to_destroy_bucket({.id = proxy.buffers[i].get_hw_staging(), .type = destroy_data_type::resource}, safe_bucket);
				add_to_destroy_bucket({.id = proxy.buffers[i].get_hw_gpu(), .type = destroy_data_type::resource}, safe_bucket);
			}
		}
		proxy = {};
	}

	void proxy_manager::destroy_target_bucket(uint8 index)
	{
		destroy_bucket& bucket = _destroy_bucket[index];
		if (bucket.list.empty())
			return;

		gfx_backend* backend = gfx_backend::get();

		for (const destroy_data& destroy : bucket.list)
		{
			if (destroy.type == destroy_data_type::texture)
				backend->destroy_texture(destroy.id);
			else if (destroy.type == destroy_data_type::sampler)
				backend->destroy_sampler(destroy.id);
			else if (destroy.type == destroy_data_type::bind_group)
				backend->destroy_bind_group(destroy.id);
			else if (destroy.type == destroy_data_type::resource)
				backend->destroy_resource(destroy.id);
			else if (destroy.type == destroy_data_type::shader)
				backend->destroy_shader(destroy.id);
		}
		bucket.list.resize(0);
	}

	void proxy_manager::add_to_destroy_bucket(const destroy_data& data, uint8 index)
	{
		destroy_bucket& bucket = _destroy_bucket[index];
		bucket.list.push_back(data);
	}
}