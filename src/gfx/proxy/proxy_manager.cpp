// Copyright (c) 2025 Inan Evin

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
#include "common/system_info.hpp"
#include "data/istream.hpp"
#include "world/world_constants.hpp"
#include "math/math.hpp"

namespace SFG
{

	void proxy_manager::init()
	{
		_shaders.init(MAX_WORLD_SHADERS);
		_textures.init(MAX_WORLD_TEXTURES);
		_samplers.init(MAX_WORLD_SAMPLERS);
		_materials.init(MAX_WORLD_MATERIALS);
		_meshes.init(MAX_WORLD_MESHES);
		_models.init(MAX_WORLD_MODELS);

		_entities.init(MAX_ENTITIES);
		_mesh_instances.init(MAX_ENTITIES);
		_cameras.init(MAX_ENTITIES);
		_ambients.init(1);
		_point_lights.init(MAX_ENTITIES);
		_spot_lights.init(MAX_ENTITIES);
		_dir_lights.init(MAX_ENTITIES);

		for (uint8 i = 0; i < BACK_BUFFER_COUNT + 1; i++)
		{
			_destroy_bucket[i].list.reserve(1000);
			_material_update_buckets[i].updates.reserve(256);
		}

		_aux_memory.init(1024 * 1024 * 2);
	}

	void proxy_manager::uninit()
	{
		for (auto& res : _textures)
		{
			if (res.status == 0)
				continue;

			destroy_texture(res);
		}

		for (auto& res : _samplers)
		{
			if (res.status == 0)
				continue;

			destroy_sampler(res);
		}

		for (auto& res : _shaders)
		{
			if (res.status == 0)
				continue;

			destroy_shader(res);
		}

		for (auto& res : _materials)
		{
			if (res.status == 0)
				continue;

			destroy_material(res);
		}

		for (auto& res : _meshes)
		{
			if (res.status == 0)
				continue;

			destroy_mesh(res);
		}

		flush_destroys(true);

		_entities.uninit();
		_cameras.uninit();
		_mesh_instances.uninit();
		_models.uninit();

		_shaders.uninit();
		_textures.uninit();
		_samplers.uninit();
		_materials.uninit();
		_meshes.uninit();
		_ambients.uninit();
		_spot_lights.uninit();
		_point_lights.uninit();
		_dir_lights.uninit();

		_aux_memory.uninit();
	}

	void proxy_manager::fetch_render_events(render_event_stream& stream)
	{
		auto&							  events = stream.get_events();
		render_event_stream::event_batch* batch	 = events.peek();

		istream				in;
		render_event_header header = {};

		while (batch != nullptr)
		{
			size_t		 offset = 0;
			const size_t total	= batch->size;
			in.open(batch->data, batch->size);

			while (!in.is_eof())
			{
				header.deserialize(in);
				process_event(header, in);
			}

			events.pop();
			batch = events.peek();
		}
	}

	void proxy_manager::flush_material_updates(uint8 frame_index)
	{
		material_update_bucket& bucket = _material_update_buckets[frame_index];
		if (bucket.updates.empty())
			return;

		static_vector<bind_group_pointer, 8> updates;
		gfx_backend*						 backend = gfx_backend::get();

		for (const material_update& up : bucket.updates)
		{
			updates.resize(0);
			render_proxy_material& mat = get_material(up.material_index);

			const uint8 texture_count = static_cast<uint8>(mat.texture_handles.size());
			for (uint8 k = 0; k < texture_count; k++)
			{
				const render_proxy_texture& txt = get_texture(mat.texture_handles[k]);
				SFG_ASSERT(txt.status);

				updates.push_back({
					.resource	   = txt.hw,
					.pointer_index = static_cast<uint8>(upi_material_texture0 + k),
					.type		   = binding_type::texture_binding,
				});
			}

			backend->bind_group_update_pointer(mat.bind_groups[frame_index], 0, updates.data(), static_cast<uint16>(updates.size()));
		}

		bucket.updates.resize(0);
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

	gfx_id proxy_manager::get_shader_variant(const render_proxy_material& mat, uint8 target_flags)
	{
		gfx_id target_shader = NULL_GFX_ID;

		for (gfx_id shader : mat.shader_handles)
		{
			const render_proxy_shader& proxy_shader = get_shader(shader);
			const bitmask<uint8>&	   flags		= proxy_shader.flags;

			if (proxy_shader.flags.is_all_set(target_flags))
			{
				target_shader = proxy_shader.hw;
				break;
			}
		}
		return target_shader;
	}

	void proxy_manager::process_event(const render_event_header& header, istream& stream)
	{
		gfx_backend* backend = gfx_backend::get();

		const world_id			index = header.index;
		const render_event_type type  = header.event_type;

		if (type == render_event_type::render_event_update_entity_transform)
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
			proxy.scale				   = ev.scale;
		}
		else if (type == render_event_type::render_event_update_entity_visibility)
		{
			render_event_entity_visibility ev = {};
			ev.deserialize(stream);
			render_proxy_entity& proxy = get_entity(index);
			proxy.status			   = render_proxy_status::rps_active;
			proxy.handle			   = index;
			proxy.flags.set(render_proxy_entity_flags::render_proxy_entity_invisible, !ev.visible);
		}
		else if (type == render_event_type::render_event_update_ambient)
		{
			render_event_ambient ev = {};
			ev.deserialize(stream);
			_peak_ambients = math::max(_peak_ambients, index) + 1;

			render_proxy_ambient& proxy = get_ambient();
			proxy.status				= render_proxy_status::rps_active;
			proxy.entity				= index;
		}
		else if (type == render_event_type::render_event_update_spot_light)
		{
			render_event_spot_light ev = {};
			ev.deserialize(stream);
			_peak_spot_lights = math::max(_peak_spot_lights, index) + 1;

			render_proxy_spot_light& proxy = _spot_lights.get(index);
			proxy.status				   = render_proxy_status::rps_active;
			proxy.entity				   = index;
		}
		else if (type == render_event_type::render_event_update_point_light)
		{
			render_event_point_light ev = {};
			ev.deserialize(stream);
			_peak_point_lights = math::max(_peak_point_lights, index) + 1;

			render_proxy_point_light& proxy = _point_lights.get(index);
			proxy.status					= render_proxy_status::rps_active;
			proxy.entity					= index;
		}
		else if (type == render_event_type::render_event_update_dir_light)
		{
			render_event_dir_light ev = {};
			ev.deserialize(stream);
			_peak_dir_lights = math::max(_peak_dir_lights, index) + 1;

			render_proxy_dir_light& proxy = _dir_lights.get(index);
			proxy.status				  = render_proxy_status::rps_active;
			proxy.entity				  = index;
		}
		else if (type == render_event_type::render_event_remove_ambient)
		{
			render_proxy_ambient& proxy = get_ambient();
			proxy.status				= render_proxy_status::rps_active;
			proxy						= {};
		}
		else if (type == render_event_type::render_event_remove_point_light)
		{
			render_proxy_point_light& proxy = _point_lights.get(index);
			proxy.status					= render_proxy_status::rps_active;
			proxy							= {};
		}
		else if (type == render_event_type::render_event_remove_spot_light)
		{
			render_proxy_spot_light& proxy = _spot_lights.get(index);
			proxy.status				   = render_proxy_status::rps_active;
			proxy						   = {};
		}
		else if (type == render_event_type::render_event_remove_dir_light)
		{
			render_proxy_dir_light& proxy = _dir_lights.get(index);
			proxy.status				  = render_proxy_status::rps_active;
			proxy						  = {};
		}
		else if (type == render_event_type::render_event_update_mesh_instance)
		{
			render_event_mesh_instance ev = {};
			ev.deserialize(stream);
			_peak_mesh_instances = math::max(_peak_mesh_instances, index) + 1;

			render_proxy_mesh_instance& proxy = get_mesh_instance(index);

			proxy.status = render_proxy_status::rps_active;
			proxy.entity = ev.entity_index;
			proxy.mesh	 = ev.mesh;
			proxy.model	 = ev.model;
		}
		else if (type == render_event_type::render_event_remove_mesh_instance)
		{
			render_proxy_mesh_instance& proxy = get_mesh_instance(index);
			proxy							  = {};
		}
		else if (type == render_event_type::render_event_update_camera)
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
		}
		else if (type == render_event_type::render_event_set_main_camera)
		{
			render_proxy_camera& proxy = get_camera(index);
			_main_camera_trait		   = index;
		}
		else if (type == render_event_type::render_event_remove_camera)
		{
			render_proxy_camera& proxy = get_camera(index);
			if (index == _main_camera_trait)
				_main_camera_trait = NULL_WORLD_ID;

			proxy = {};
		}
		else if (type == render_event_type::render_event_create_texture)
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
				.views			= {{}},
				.mip_levels		= static_cast<uint8>(ev.buffers.size()),
				.array_length	= 1,
				.samples		= 1,
#ifndef SFG_STRIP_DEBUG_NAMES
				.debug_name = ev.name.c_str(),
#endif
			});

			proxy.intermediate = backend->create_resource({
				.size		= ev.intermediate_size,
				.flags		= resource_flags::rf_cpu_visible,
				.debug_name = "texture_intermediate",
			});

			_texture_queue.add_request(ev.buffers, proxy.hw, proxy.intermediate, 1, resource_state::ps_resource);

#ifndef SFG_STRIP_DEBUG_NAMES
			SFG_TRACE("Created texture proxy for: {0}", ev.name);
#endif
		}
		else if (type == render_event_type::render_event_destroy_texture)
		{
			render_proxy_texture& proxy = get_texture(index);
			destroy_texture(proxy);
		}
		else if (type == render_event_type::render_event_reload_texture)
		{
			render_event_resource_reloaded ev = {};
			ev.deserialize(stream);
			for (uint32 i = 0; i < MAX_WORLD_MATERIALS; i++)
			{
				render_proxy_material& mat = _materials.get(i);
				if (mat.status != render_proxy_status::rps_active)
					continue;

				bool changed = false;
				for (resource_id& txt : mat.texture_handles)
				{
					if (txt == ev.prev_id)
					{
						changed = true;
						txt		= ev.new_id;
					}
				}

				if (!changed)
					continue;

				for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
					_material_update_buckets[i].updates.push_back({.material_index = i});
			}
		}
		else if (type == render_event_type::render_event_create_sampler)
		{
			render_event_sampler ev = {};
			ev.deserialize(stream);
			render_proxy_sampler& proxy = get_sampler(index);
			proxy.status				= render_proxy_status::rps_active;
			proxy.handle				= index;
			proxy.hw					= backend->create_sampler(ev.desc);

#ifndef SFG_STRIP_DEBUG_NAMES
			SFG_TRACE("Created sampler proxy for: {0}", ev.desc.debug_name);

#endif

			// Invalidate materials using this texture.
			// Materials are dependent on shaders & textures. Shaders already checked in runtime.
			// This is to avoid checking textures too.
			for (render_proxy_material& m : _materials)
			{
				for (uint16 txt : m.texture_handles)
				{
					if (txt == index)
					{
						m.status = render_proxy_status::rps_obsolete;
						break;
					}
				}
			}
		}
		else if (type == render_event_type::render_event_destroy_sampler)
		{
			render_proxy_sampler& proxy = get_sampler(index);
			destroy_sampler(proxy);
		}
		else if (type == render_event_type::render_event_create_shader)
		{
			render_event_shader ev = {};
			ev.deserialize(stream);
			render_proxy_shader& proxy = get_shader(index);
			proxy.status			   = render_proxy_status::rps_active;
			proxy.handle			   = index;
			proxy.hw				   = backend->create_shader(ev.desc);
			proxy.flags				   = ev.flags;

#ifndef SFG_STRIP_DEBUG_NAMES
			SFG_TRACE("Created shader proxy for: {0}", ev.desc.debug_name);
#endif
			ev.desc.destroy();
		}
		else if (type == render_event_type::render_event_destroy_shader)
		{
			render_proxy_shader& proxy = get_shader(index);
			destroy_shader(proxy);
		}
		else if (type == render_event_type::render_event_reload_shader)
		{
			render_event_resource_reloaded ev = {};
			ev.deserialize(stream);
			for (uint32 i = 0; i < MAX_WORLD_MATERIALS; i++)
			{
				render_proxy_material& mat = _materials.get(i);
				if (mat.status != render_proxy_status::rps_active)
					continue;

				for (resource_id& shader : mat.shader_handles)
				{
					if (shader == ev.prev_id)
						shader = ev.new_id;
				}
			}
		}
		else if (type == render_event_type::render_event_create_material)
		{
			render_event_material ev = {};
			ev.deserialize(stream);

			render_proxy_material& proxy = get_material(index);
			proxy.status				 = render_proxy_status::rps_active;
			proxy.handle				 = index;
			proxy.flags					 = ev.flags;

			for (resource_handle h : ev.shaders)
				proxy.shader_handles.push_back(h.index);

			for (resource_handle h : ev.textures)
				proxy.texture_handles.push_back(h.index);

			for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
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

				proxy.bind_groups[i] = backend->create_empty_bind_group();

				backend->bind_group_add_pointer(proxy.bind_groups[i], root_param_index::rpi_table_material, upi_material_texture4 + 1, false);

				vector<bind_group_pointer> updates;
				updates.push_back({
					.resource	   = proxy.buffers[i].get_hw_gpu(),
					.pointer_index = upi_material_ubo0,
					.type		   = binding_type::ubo,
				});

				const uint8 texture_count = static_cast<uint8>(ev.textures.size());
				for (uint8 k = 0; k < texture_count; k++)
				{
					const render_proxy_texture& txt = get_texture(ev.textures[k].index);
					SFG_ASSERT(txt.status);

					updates.push_back({
						.resource	   = txt.hw,
						.pointer_index = static_cast<uint8>(upi_material_texture0 + k),
						.type		   = binding_type::texture_binding,
					});
				}
				backend->bind_group_update_pointer(proxy.bind_groups[i], 0, updates);

				if (ev.use_sampler)
				{
					backend->bind_group_add_pointer(proxy.bind_groups[i], root_param_index::rpi_table_dyn_sampler, 4, true);
					backend->bind_group_update_pointer(proxy.bind_groups[i],
													   1,
													   {
														   {.resource = _samplers.get(ev.sampler_index).hw, .pointer_index = static_cast<uint8>(upi_dyn_sampler0), .type = binding_type::sampler},
														   {.resource = _samplers.get(ev.sampler_index).hw, .pointer_index = static_cast<uint8>(upi_dyn_sampler1), .type = binding_type::sampler},
														   {.resource = _samplers.get(ev.sampler_index).hw, .pointer_index = static_cast<uint8>(upi_dyn_sampler2), .type = binding_type::sampler},
														   {.resource = _samplers.get(ev.sampler_index).hw, .pointer_index = static_cast<uint8>(upi_dyn_sampler3), .type = binding_type::sampler},
													   });
				}

				proxy.buffers[i].buffer_data(0, ev.data.data, ev.data.size);
				_buffer_queue.add_request({
					.buffer	  = &proxy.buffers[i],
					.to_state = resource_state::vertex_cbv,
				});
			}

			SFG_FREE(ev.data.data);

#ifndef SFG_STRIP_DEBUG_NAMES
			SFG_TRACE("Created material proxy for: {0}", ev.name);
#endif
		}
		else if (type == render_event_type::render_event_update_material)
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
						.to_state  = resource_state::vertex_cbv,
					},
					i);
			}
		}
		else if (type == render_event_type::render_event_destroy_material)
		{
			render_proxy_material& proxy = get_material(index);
			destroy_material(proxy);
		}
		else if (type == render_event_type::render_event_create_model)
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

#ifndef SFG_STRIP_DEBUG_NAMES
			SFG_TRACE("Created model proxy for: {0}", ev.name);
#endif
		}
		else if (type == render_event_type::render_event_destroy_model)
		{
			render_proxy_model& proxy = get_model(index);
			destroy_model(proxy);
		}
		else if (type == render_event_type::render_event_create_mesh)
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
					.to_state = resource_state::vertex_cbv,
				});
				_buffer_queue.add_request({
					.buffer	  = &proxy.index_buffer,
					.to_state = resource_state::index_buffer,
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

#ifndef SFG_STRIP_DEBUG_NAMES
			SFG_TRACE("Created mesh proxy for: {0}", ev.name);
#endif
		}
		else if (type == render_event_type::render_event_destroy_mesh)
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
		add_to_destroy_bucket({.id = proxy.hw, .type = destroy_data_type::shader}, safe_bucket);
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

	void proxy_manager::destroy_material(render_proxy_material& proxy)
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			const uint8 safe_bucket = (frame_info::get_render_frame() + i) % (BACK_BUFFER_COUNT + 1);
			add_to_destroy_bucket({.id = proxy.bind_groups[i], .type = destroy_data_type::bind_group}, safe_bucket);
			add_to_destroy_bucket({.id = proxy.buffers[i].get_hw_staging(), .type = destroy_data_type::resource}, safe_bucket);
			add_to_destroy_bucket({.id = proxy.buffers[i].get_hw_gpu(), .type = destroy_data_type::resource}, safe_bucket);
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