// Copyright (c) 2025 Inan Evin

#include "material.hpp"
#include "material_raw.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/util/gfx_util.hpp"
#include "io/assert.hpp"
#include "world/world.hpp"
#include "gfx/world/world_renderer.hpp"
#include "resources/texture.hpp"
#include "resources/shader.hpp"
#include "reflection/reflection.hpp"
#include "gfx/event_stream/render_event_stream.hpp"

namespace SFG
{

	material_reflection::material_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<material>::value, "stkmat");

#ifdef SFG_TOOLMODE

		m.add_function<void*, const char*>("cook_from_file"_hs, [](const char* path) -> void* {
			material_raw* raw = new material_raw();
			if (!raw->cook_from_file(path))
			{
				delete raw;
				return nullptr;
			}

			return raw;
		});
#endif

		m.add_function<void*, istream&>("cook_from_stream"_hs, [](istream& stream) -> void* {
			material_raw* raw = new material_raw();
			raw->deserialize(stream);
			return raw;
		});

		m.add_function<resource_handle, void*, world&, string_id>("create_from_raw"_hs, [](void* raw, world& w, string_id sid) -> resource_handle {
			material_raw*	 raw_ptr   = reinterpret_cast<material_raw*>(raw);
			world_resources& resources = w.get_resources();
			resource_handle	 handle	   = resources.add_resource<material>(sid);
			material&		 res	   = resources.get_resource<material>(handle);
			res.create_from_raw(*raw_ptr, resources);
			delete raw_ptr;

			w.get_renderer()->get_resource_uploads().add_pending_material(&res);

			return handle;
		});

		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<material>(MAX_WORLD_MATERIALS); });

		m.add_function<void, world&, resource_handle>("destroy"_hs, [](world& w, resource_handle h) -> void {
			world_resources& res = w.get_resources();
			res.get_resource<material>(h).destroy();
			res.remove_resource<material>(h);
		});

		m.add_function<void, void*, ostream&>("serialize"_hs, [](void* loader, ostream& stream) -> void {
			material_raw* raw = reinterpret_cast<material_raw*>(loader);
			raw->serialize(stream);
		});
	}

	void material::create_from_raw(const material_raw& raw, world_resources& resources)
	{
		gfx_backend* backend = gfx_backend::get();

		const uint8 texture_count = static_cast<uint8>(raw.textures.size());

		_material_data = raw.material_data;
		_flags.set(material::flags::is_opaque, raw.is_opaque);
		_flags.set(material::flags::is_forward, raw.is_forward);
		SFG_ASSERT(!raw.shaders.empty());

		for (string_id sh : raw.shaders)
			_all_shaders.push_back(resources.get_resource_handle_by_hash<shader>(sh));
		_default_shader = resources.get_resource<shader>(_all_shaders[0]).get_hw();

		for (uint8 i = 0; i < FRAMES_IN_FLIGHT; i++)
		{
			if (_buffers[i].is_alive())
			{
				_buffers[i].destroy();
				backend->destroy_bind_group(_bind_groups[i]);
			}

			_buffers[i].create_staging_hw(
				{
					.size		= static_cast<uint32>(raw.material_data.get_size()),
					.flags		= resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible,
					.debug_name = "material_data",
				},
				{
					.size		= static_cast<uint32>(raw.material_data.get_size()),
					.flags		= resource_flags::rf_constant_buffer | resource_flags::rf_gpu_only,
					.debug_name = "material_data",
				});

			// uploader should take care of this.
			// _buffers[i].buffer_data(0, raw.material_data.get_raw(), raw.material_data.get_size());

			_bind_groups[i] = backend->create_empty_bind_group();
			backend->bind_group_add_pointer(_bind_groups[i], rpi_table_material, 3, false);

			vector<bind_group_pointer> updates;
			updates.push_back({
				.resource	   = _buffers[i].get_hw_gpu(),
				.pointer_index = upi_material_ubo0,
				.type		   = binding_type::ubo,
			});

			// for (uint8 k = 0; k < texture_count; k++)
			// {
			// 	updates.push_back({
			// 		.resource	   = resources.get_resource_by_hash<texture>(raw.textures[k]).get_hw(),
			// 		.pointer_index = static_cast<uint8>(upi_material_texture0 + k),
			// 		.type		   = binding_type::texture_binding,
			// 	});
			// }
			backend->bind_group_update_pointer(_bind_groups[i], 0, updates);
		}
	}

	void material::push_create_event(render_event_stream& stream, resource_handle handle)
	{
		// stream.add_event({
		// 	.create_callback =
		// 		[data = this->_material_data](void* storage) {
		//
		// 		},
		// 	.handle		= handle,
		// 	.event_type = render_event_type::render_event_create_material,
		// });
	}

	void material::destroy()
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint8 i = 0; i < FRAMES_IN_FLIGHT; i++)
		{
			SFG_ASSERT(_buffers[i].is_alive());
			_buffers[i].destroy();
			backend->destroy_bind_group(_bind_groups[i]);
		}
	}

	gfx_id material::get_shader(world_resources& resources, uint8 flags_to_match) const
	{
		if (flags_to_match == 0)
			return _default_shader;

		for (pool_handle16 handle : _all_shaders)
		{
			const shader& sh = resources.get_resource<shader>(handle);
			if (sh.get_flags().is_set(flags_to_match))
				return sh.get_hw();
		}
		SFG_ASSERT(false);
		return _default_shader;
	}
}