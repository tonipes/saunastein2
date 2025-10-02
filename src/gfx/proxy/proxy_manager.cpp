// Copyright (c) 2025 Inan Evin

#include "proxy_manager.hpp"
#include "resources/common_resources.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_event_storage_gfx.hpp"
#include "gfx/texture_queue.hpp"
#include "gfx/buffer_queue.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/util/gfx_util.hpp"
#include "common/system_info.hpp"

namespace SFG
{

	void proxy_manager::init()
	{
		_shaders.init(MAX_WORLD_SHADERS);
		_textures.init(MAX_WORLD_TEXTURES);
		_samplers.init(MAX_WORLD_SAMPLERS);
		_materials.init(MAX_WORLD_MATERIALS);
		_meshes.init(MAX_WORLD_MESHES);

		for (uint8 i = 0; i < FRAMES_IN_FLIGHT + 1; i++)
			_destroy_bucket[i].list.reserve(1000);

		_aux_memory.init(1024 * 1024 * 2);
	}

	void proxy_manager::uninit()
	{
		for (auto& res : _textures)
		{
			if (res.active == 0)
				continue;

			destroy_texture(res);
		}

		for (auto& res : _samplers)
		{
			if (res.active == 0)
				continue;

			destroy_sampler(res);
		}

		for (auto& res : _shaders)
		{
			if (res.active == 0)
				continue;

			destroy_shader(res);
		}

		for (auto& res : _materials)
		{
			if (res.active == 0)
				continue;

			destroy_material(res);
		}

		for (auto& res : _meshes)
		{
			if (res.active == 0)
				continue;

			destroy_mesh(res);
		}

		flush_destroys(true);

		_shaders.uninit();
		_textures.uninit();
		_samplers.uninit();
		_materials.uninit();
		_meshes.uninit();

		_aux_memory.uninit();
	}

	void proxy_manager::fetch_render_events(render_event_stream& stream)
	{
		auto&		  events  = stream.get_events();
		render_event* ev	  = events.peek();
		gfx_backend*  backend = gfx_backend::get();

		while (ev != nullptr)
		{
			uint16 index = ev->header.handle.index;

			if (ev->header.event_type == render_event_type::render_event_create_texture)
			{
				render_event_storage_texture* stg = reinterpret_cast<render_event_storage_texture*>(ev->data);

				render_proxy_texture& proxy = get_texture(index);
				proxy.active				= 1;
				proxy.handle				= index;

				proxy.hw = backend->create_texture({
					.texture_format = static_cast<format>(stg->format),
					.size			= stg->size,
					.flags			= texture_flags::tf_is_2d | texture_flags::tf_sampled,
					.views			= {{}},
					.mip_levels		= static_cast<uint8>(stg->buffers.size()),
					.array_length	= 1,
					.samples		= 1,
					.debug_name		= stg->name,
				});

				if (stg->name != nullptr)
					SFG_FREE((void*)stg->name);

				proxy.intermediate = backend->create_resource({
					.size		= stg->intermediate_size,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "texture_intermediate",
				});

				_texture_queue.add_request(stg->buffers, proxy.hw, proxy.intermediate, 1);
			}
			else if (ev->header.event_type == render_event_type::render_event_destroy_texture)
			{
				render_proxy_texture& proxy = get_texture(index);
				destroy_texture(proxy);
			}
			else if (ev->header.event_type == render_event_type::render_event_create_sampler)
			{
				render_event_storage_sampler* stg	= reinterpret_cast<render_event_storage_sampler*>(ev->data);
				render_proxy_sampler		  proxy = get_sampler(index);
				proxy.active						= 1;
				proxy.handle						= index;
				proxy.hw							= backend->create_sampler(stg->desc);

				if (stg->name != nullptr)
					SFG_FREE((void*)stg->name);
			}
			else if (ev->header.event_type == render_event_type::render_event_destroy_sampler)
			{
				render_event_storage_sampler* stg	= reinterpret_cast<render_event_storage_sampler*>(ev->data);
				render_proxy_sampler		  proxy = get_sampler(index);
				destroy_sampler(proxy);
			}
			else if (ev->header.event_type == render_event_type::render_event_create_shader)
			{
				render_event_storage_shader* stg   = reinterpret_cast<render_event_storage_shader*>(ev->data);
				render_proxy_shader			 proxy = get_shader(index);
				proxy.active					   = 1;
				proxy.handle					   = index;
				proxy.hw						   = backend->create_shader(stg->desc);

				stg->desc.destroy();

				if (stg->name != nullptr)
					SFG_FREE((void*)stg->name);
			}
			else if (ev->header.event_type == render_event_type::render_event_destroy_shader)
			{
				render_event_storage_shader* stg   = reinterpret_cast<render_event_storage_shader*>(ev->data);
				render_proxy_shader			 proxy = get_shader(index);
				destroy_shader(proxy);
			}
			else if (ev->header.event_type == render_event_type::render_event_create_material)
			{
				render_event_storage_material* stg	 = reinterpret_cast<render_event_storage_material*>(ev->data);
				render_proxy_material		   proxy = get_material(index);
				proxy.active						 = 1;
				proxy.handle						 = index;

				for (uint8 i = 0; i < FRAMES_IN_FLIGHT; i++)
				{
					proxy.buffers[i].create_staging_hw(
						{
							.size		= static_cast<uint32>(stg->data.get_size()),
							.flags		= resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible,
							.debug_name = stg->name,
						},
						{
							.size		= static_cast<uint32>(stg->data.get_size()),
							.flags		= resource_flags::rf_constant_buffer | resource_flags::rf_gpu_only,
							.debug_name = stg->name,
						});

					if (stg->name != nullptr)
						SFG_FREE((void*)stg->name);

					proxy.bind_groups[i] = backend->create_empty_bind_group();
					backend->bind_group_add_pointer(proxy.bind_groups[i], root_param_index::rpi_table_material, 3, false);

					vector<bind_group_pointer> updates;
					updates.push_back({
						.resource	   = proxy.buffers[i].get_hw_gpu(),
						.pointer_index = upi_material_ubo0,
						.type		   = binding_type::ubo,
					});

					const uint8 texture_count = static_cast<uint8>(stg->textures.size());
					for (uint8 k = 0; k < texture_count; k++)
					{
						updates.push_back({
							.resource	   = get_texture(stg->textures[k].index).hw,
							.pointer_index = static_cast<uint8>(upi_material_texture0 + k),
							.type		   = binding_type::texture_binding,
						});
					}

					backend->bind_group_update_pointer(proxy.bind_groups[i], 0, updates);

					proxy.buffers[i].buffer_data(0, stg->data.get_raw(), stg->data.get_size());
					_buffer_queue.add_request({.buffer = &proxy.buffers[i]});
				}
			}
			else if (ev->header.event_type == render_event_type::render_event_update_material)
			{
				render_event_storage_material* stg	 = reinterpret_cast<render_event_storage_material*>(ev->data);
				render_proxy_material		   proxy = get_material(index);

				for (uint8 i = 0; i < FRAMES_IN_FLIGHT; i++)
				{
					_buffer_queue.add_request(
						{
							.buffer	   = &proxy.buffers[i],
							.data	   = proxy.data.get_raw(),
							.data_size = proxy.data.get_size(),
						},
						i);
				}

				events.pop();
				ev = events.peek();
			}
			else if (ev->header.event_type == render_event_type::render_event_create_mesh)
			{
				render_event_storage_mesh* stg	 = reinterpret_cast<render_event_storage_mesh*>(ev->data);
				render_proxy_mesh		   proxy = get_mesh(index);

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
							.size		= static_cast<uint32>(vertex_size),
							.flags		= resource_flags::rf_cpu_visible,
							.debug_name = "vertex_buffer_staging",
						},
						{
							.size		= static_cast<uint32>(vertex_size),
							.flags		= resource_flags::rf_vertex_buffer | resource_flags::rf_gpu_only,
							.debug_name = "vertex_buffer",
						});

					proxy.index_buffer.create_staging_hw(
						{
							.size		= static_cast<uint32>(index_size),
							.flags		= resource_flags::rf_cpu_visible,
							.debug_name = "index_buffer_staging",
						},
						{
							.size		= static_cast<uint32>(index_size),
							.flags		= resource_flags::rf_index_buffer | resource_flags::rf_gpu_only,
							.debug_name = "index_buffer",
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
						const size_t			vertices   = p.vertices.size();
						const size_t			indices	   = p.indices.size();
						const size_t			vtx_sz	   = p.vertices.size() * vtx_type_size;
						const size_t			idx_sz	   = p.indices.size() * sizeof(primitive_index);

						proxy_prim.vertex_start = vtx_counter;
						proxy_prim.index_start	= idx_counter;
						proxy_prim.index_count	= static_cast<uint32>(p.indices.size());

						proxy.vertex_buffer.buffer_data(vertex_offset, p.vertices.data(), vtx_sz);
						proxy.index_buffer.buffer_data(index_offset, p.indices.data(), idx_sz);

						vertex_offset += vtx_sz;
						index_offset += idx_sz;
						vtx_counter += static_cast<uint32>(p.vertices.size());
						idx_counter += proxy_prim.index_count;
					}
				};

				if (!stg->primitives_static.empty())
				{
					process(stg->primitives_static, sizeof(vertex_static));
				}
				else if (!stg->primitives_skinned.empty())
				{
					process(stg->primitives_skinned, sizeof(vertex_skinned));
				}
				else
				{
					SFG_ASSERT(false);
				}
			}
			else if (ev->header.event_type == render_event_type::render_event_destroy_mesh)
			{
				render_event_storage_mesh* stg	 = reinterpret_cast<render_event_storage_mesh*>(ev->data);
				render_proxy_mesh		   proxy = get_mesh(index);
				destroy_mesh(proxy);
			}
		}
	}

	void proxy_manager::flush_destroys(bool force)
	{
		gfx_backend* backend = gfx_backend::get();

		const uint8 mod			= FRAMES_IN_FLIGHT + 1;
		const uint8 safe_bucket = frame_info::get_render_frame() % mod;
		destroy_target_bucket(safe_bucket);
		if (force)
		{
			for (uint32 i = 1; i < mod; i++)
			{
				const uint8 other = (safe_bucket + i) % mod;
				destroy_target_bucket(other);
			}
		}
	}

	void proxy_manager::destroy_texture(render_proxy_texture& proxy)
	{
		const uint8 safe_bucket = frame_info::get_render_frame() % (FRAMES_IN_FLIGHT + 1);
		add_to_destroy_bucket({.id = proxy.hw, .type = destroy_data_type::texture}, safe_bucket);
		add_to_destroy_bucket({.id = proxy.intermediate, .type = destroy_data_type::resource}, safe_bucket);
		proxy = {};
	}

	void proxy_manager::destroy_sampler(render_proxy_sampler& proxy)
	{
		const uint8 safe_bucket = frame_info::get_render_frame() % (FRAMES_IN_FLIGHT + 1);
		add_to_destroy_bucket({.id = proxy.hw, .type = destroy_data_type::sampler}, safe_bucket);
		proxy = {};
	}

	void proxy_manager::destroy_shader(render_proxy_shader& proxy)
	{
		const uint8 safe_bucket = frame_info::get_render_frame() % (FRAMES_IN_FLIGHT + 1);
		add_to_destroy_bucket({.id = proxy.hw, .type = destroy_data_type::shader}, safe_bucket);
		proxy = {};
	}

	void proxy_manager::destroy_mesh(render_proxy_mesh& proxy)
	{
		const uint8 safe_bucket = frame_info::get_render_frame() % (FRAMES_IN_FLIGHT + 1);
		add_to_destroy_bucket({.id = proxy.vertex_buffer.get_hw_staging(), .type = destroy_data_type::resource}, safe_bucket);
		add_to_destroy_bucket({.id = proxy.vertex_buffer.get_hw_gpu(), .type = destroy_data_type::resource}, safe_bucket);
		add_to_destroy_bucket({.id = proxy.index_buffer.get_hw_staging(), .type = destroy_data_type::resource}, safe_bucket);
		add_to_destroy_bucket({.id = proxy.index_buffer.get_hw_gpu(), .type = destroy_data_type::resource}, safe_bucket);
		_aux_memory.free(proxy.primitives);
		proxy = {};
	}

	void proxy_manager::destroy_material(render_proxy_material& proxy)
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint8 i = 0; i < FRAMES_IN_FLIGHT; i++)
		{
			const uint8 safe_bucket = (frame_info::get_render_frame() + i) % (FRAMES_IN_FLIGHT + 1);
			add_to_destroy_bucket({.id = proxy.bind_groups[i], .type = destroy_data_type::texture}, safe_bucket);
			add_to_destroy_bucket({.id = proxy.buffers->get_hw_staging(), .type = destroy_data_type::resource}, safe_bucket);
			add_to_destroy_bucket({.id = proxy.buffers->get_hw_gpu(), .type = destroy_data_type::resource}, safe_bucket);
		}
		proxy = {};
	}

	void proxy_manager::destroy_target_bucket(uint8 index)
	{
		gfx_backend* backend = gfx_backend::get();

		destroy_bucket& bucket = _destroy_bucket[index];

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