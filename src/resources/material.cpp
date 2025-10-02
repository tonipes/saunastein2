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
#include "gfx/event_stream/render_event_storage_gfx.hpp"

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
			res.create_from_raw(*raw_ptr, resources, w.get_render_stream(), handle);
			delete raw_ptr;
			return handle;
		});

		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<material>(MAX_WORLD_MATERIALS); });

		m.add_function<void, world&, resource_handle>("destroy"_hs, [](world& w, resource_handle handle) -> void {
			world_resources& res = w.get_resources();
			res.get_resource<material>(handle).destroy(w.get_render_stream(), handle);
			res.remove_resource<material>(handle);
		});

		m.add_function<void, void*, ostream&>("serialize"_hs, [](void* loader, ostream& stream) -> void {
			material_raw* raw = reinterpret_cast<material_raw*>(loader);
			raw->serialize(stream);
		});
	}

	void material::create_from_raw(const material_raw& raw, world_resources& resources, render_event_stream& stream, resource_handle handle)
	{
		gfx_backend* backend = gfx_backend::get();

		const uint8 texture_count = static_cast<uint8>(raw.textures.size());
		_material_data			  = raw.material_data;
		_flags.set(material::flags::is_opaque, raw.is_opaque);
		_flags.set(material::flags::is_forward, raw.is_forward);
		SFG_ASSERT(!raw.shaders.empty());

		for (string_id sh : raw.shaders)
		{
			const resource_handle shader_handle = resources.get_resource_handle_by_hash<shader>(sh);
			_all_shaders.push_back(shader_handle);
			shader& shd = resources.get_resource<shader>(shader_handle);
			_all_shader_flags.push_back(shd.get_flags());
		}

		render_event ev = {.header = {
							   .handle	   = handle,
							   .event_type = render_event_type::render_event_create_material,
						   }};

		render_event_storage_material* stg = reinterpret_cast<render_event_storage_material*>(ev.data);
		stg->name						   = reinterpret_cast<const char*>(SFG_MALLOC(strlen(raw.name.c_str())));
		stg->data						   = _material_data;

		for (string_id txt : raw.textures)
			stg->textures.push_back(resources.get_resource_handle_by_hash<texture>(txt));

		if (stg->name != nullptr)
			strcpy((char*)stg->name, raw.name.c_str());

		stream.add_event(ev);
	}

	void material::destroy(render_event_stream& stream, resource_handle handle)
	{
		stream.add_event({.header = {
							  .handle	  = handle,
							  .event_type = render_event_type::render_event_destroy_material,
						  }});
	}

	resource_handle material::get_shader(uint8 flags_to_match) const
	{
		if (flags_to_match == 0)
			return _all_shaders[0];

		uint16 i = 0;
		for (const bitmask<uint8>& flags : _all_shader_flags)
		{
			if (flags.is_set(flags_to_match))
				return _all_shaders[i];
			i++;
		}
		SFG_ASSERT(false);
		return _all_shaders[0];
	}

	void material::update_data(render_event_stream& stream, resource_handle handle)
	{
		render_event				   ev  = {.header = {
												  .handle	  = handle,
												  .event_type = render_event_type::render_event_update_material,
							  }};
		render_event_storage_material* stg = reinterpret_cast<render_event_storage_material*>(ev.data);
		stg->data						   = _material_data;
		stream.add_event(ev);
	}
}