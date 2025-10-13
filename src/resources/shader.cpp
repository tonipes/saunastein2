// Copyright (c) 2025 Inan Evin

#include "shader.hpp"
#include "shader_raw.hpp"
#include "gfx/renderer.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "world/world.hpp"
#include "reflection/reflection.hpp"

#ifdef SFG_TOOLMODE
#include "project/engine_data.hpp"
#endif

namespace SFG
{

	shader_reflection::shader_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<shader>::value, type_id<shader>::index, "stkshader");

#ifdef SFG_TOOLMODE

		m.add_function<void*, const char*, world&>("cook_from_file"_hs, [](const char* path, world& w) -> void* {
			shader_raw* raw = new shader_raw();
			if (!raw->cook_from_file(path, false, renderer::get_bind_layout_global()))
			{
				delete raw;
				return nullptr;
			}

			return raw;
		});

		m.add_function<void, void*, vector<string>&>("get_dependencies"_hs, [](void* loader, vector<string>& out) {
			shader_raw* raw = reinterpret_cast<shader_raw*>(loader);
			out.push_back(raw->source);
		});
#endif

		m.add_function<void*, istream&>("cook_from_stream"_hs, [](istream& stream) -> void* {
			shader_raw* raw = new shader_raw();
			raw->deserialize(stream, false, renderer::get_bind_layout_global());
			return raw;
		});

		m.add_function<resource_handle, void*, world&>("create_from_raw"_hs, [](void* raw, world& w) -> resource_handle {
			shader_raw*		 raw_ptr   = reinterpret_cast<shader_raw*>(raw);
			world_resources& resources = w.get_resources();
			resource_handle	 handle	   = resources.add_resource<shader>(TO_SID(raw_ptr->name));
			shader&			 res	   = resources.get_resource<shader>(handle);
			res.create_from_raw(*raw_ptr, w.get_render_stream(), resources.get_aux(), handle);
			delete raw_ptr;
			return handle;
		});

		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<shader>(MAX_WORLD_SHADERS); });

		m.add_function<void, world&, resource_handle>("destroy"_hs, [](world& w, resource_handle handle) -> void {
			world_resources& res = w.get_resources();
			res.get_resource<shader>(handle).destroy(w.get_render_stream(), res.get_aux(), handle);
			res.remove_resource<shader>(handle);
		});

		m.add_function<void, void*, ostream&>("serialize"_hs, [](void* loader, ostream& stream) -> void {
			shader_raw* raw = reinterpret_cast<shader_raw*>(loader);
			raw->serialize(stream);
		});
	}

	void shader::create_from_raw(const shader_raw& raw, render_event_stream& stream, chunk_allocator32& alloc, resource_handle handle)
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		_flags.set(raw.is_skinned, res_shader_flags::res_shader_flags_is_skinned);

		render_event_shader ev = {};
		ev.desc				   = raw.desc;
		ev.flags			   = _flags.value();

		stream.add_event(
			{
				.index		= static_cast<uint32>(handle.index),
				.event_type = render_event_type::render_event_create_shader,
			},
			ev);
	}

	void shader::destroy(render_event_stream& stream, chunk_allocator32& alloc, resource_handle handle)
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		stream.add_event({
			.index		= static_cast<uint32>(handle.index),
			.event_type = render_event_type::render_event_destroy_shader,
		});
	}
}
