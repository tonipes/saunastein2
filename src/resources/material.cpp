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
#include "gfx/event_stream/render_events_gfx.hpp"

#ifdef SFG_TOOLMODE
#include "project/engine_data.hpp"
#endif

namespace SFG
{
	material_reflection::material_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<material>::value, type_id<material>::index, "stkmat");

#ifdef SFG_TOOLMODE

		m.add_function<void*, const char*, world&>("cook_from_file"_hs, [](const char* path, world& w) -> void* {
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

		m.add_function<resource_handle, void*, world&>("create_from_raw_2"_hs, [](void* raw, world& w) -> resource_handle {
			material_raw*	 raw_ptr   = reinterpret_cast<material_raw*>(raw);
			world_resources& resources = w.get_resources();
			resource_handle	 handle	   = resources.add_resource<material>(TO_SID(raw_ptr->name));
			material&		 res	   = resources.get_resource<material>(handle);

			resource_handle sampler_handle = {};
			if (raw_ptr->use_sampler_definition)
				sampler_handle = resources.get_or_add_sampler(raw_ptr->sampler_definition);

			res.create_from_raw(*raw_ptr, resources, resources.get_aux(), w.get_render_stream(), handle, sampler_handle);
			delete raw_ptr;
			return handle;
		});

		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<material>(MAX_WORLD_MATERIALS); });

		m.add_function<void, world&, resource_handle>("destroy"_hs, [](world& w, resource_handle handle) -> void {
			world_resources& res = w.get_resources();
			res.get_resource<material>(handle).destroy(w.get_render_stream(), res.get_aux(), handle);
			res.remove_resource<material>(handle);
		});

		m.add_function<void, void*, ostream&>("serialize"_hs, [](void* loader, ostream& stream) -> void {
			material_raw* raw = reinterpret_cast<material_raw*>(loader);
			raw->serialize(stream);
		});
	}

	void material::create_from_raw(const material_raw& raw, world_resources& resources, chunk_allocator32& alloc, render_event_stream& stream, resource_handle handle, resource_handle sampler_handle)
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif
		gfx_backend* backend = gfx_backend::get();

		const uint8 texture_count = static_cast<uint8>(raw.textures.size());
		_material_data			  = raw.material_data;
		_flags.set(material_flags::material_flags_is_gbuffer, raw.pass_mode == material_pass_mode::gbuffer);
		_flags.set(material_flags::material_flags_is_gbuffer_discard, raw.pass_mode == material_pass_mode::gbuffer_transparent);
		_flags.set(material_flags::material_flags_is_forward, raw.pass_mode == material_pass_mode::forward);
		SFG_ASSERT(!raw.shaders.empty());

		render_event_material stg = {};
		stg.data.data			  = reinterpret_cast<uint8*>(SFG_MALLOC(_material_data.get_size()));
		SFG_MEMCPY(stg.data.data, _material_data.get_raw(), _material_data.get_size());
		stg.data.size	  = _material_data.get_size();
		stg.flags		  = _flags.value();
		stg.use_sampler	  = !sampler_handle.is_null();
		stg.sampler_index = sampler_handle.index;

		for (string_id sid : raw.textures)
		{
			const resource_handle handle = resources.get_resource_handle_by_hash<texture>(sid);
			stg.textures.push_back(handle);
		}

		for (string_id sid : raw.shaders)
		{
			const resource_handle handle = resources.get_resource_handle_by_hash<shader>(sid);
			stg.shaders.push_back(handle);
		}

#ifndef SFG_STRIP_DEBUG_NAMES
		stg.name = raw.name;
#endif

		stream.add_event(
			{
				.index		= static_cast<uint32>(handle.index),
				.event_type = render_event_type::render_event_create_material,
			},
			stg);
	}

	void material::destroy(render_event_stream& stream, chunk_allocator32& alloc, resource_handle handle)
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		stream.add_event({
			.index		= static_cast<uint32>(handle.index),
			.event_type = render_event_type::render_event_destroy_material,
		});

		_material_data.destroy();
	}

	void material::update_data(render_event_stream& stream, resource_handle handle)
	{
		stream.add_event({
			.index		= static_cast<uint32>(handle.index),
			.event_type = render_event_type::render_event_update_material,
		});
	}
}