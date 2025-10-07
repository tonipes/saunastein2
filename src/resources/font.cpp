// Copyright (c) 2025 Inan Evin

#include "font.hpp"
#include "font_raw.hpp"
#include "gui/vekt.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"

#ifdef SFG_TOOLMODE
#include "project/engine_data.hpp"
#endif

namespace SFG
{
	font_reflection g_font_reflection;

	font_reflection::font_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<font>::value, type_id<font>::index, "stkfont");

#ifdef SFG_TOOLMODE

		m.add_function<void*, const char*, world&>("cook_from_file"_hs, [](const char* path, world& w) -> void* {
			font_raw* raw = new font_raw();
			if (!raw->cook_from_file(path))
			{
				delete raw;
				return nullptr;
			}

			world_resources&				 resources = w.get_resources();
			world_resources::resource_watch& watch	   = resources.add_resource_watch();
			watch.base_path							   = path;
			watch.dependencies.push_back(engine_data::get().get_working_dir() + raw->name);

			return raw;
		});
#endif

		m.add_function<void*, istream&>("cook_from_stream"_hs, [](istream& stream) -> void* {
			font_raw* raw = new font_raw();
			raw->deserialize(stream);
			return raw;
		});

		m.add_function<resource_handle, void*, world&, string_id>("create_from_raw"_hs, [](void* raw, world& w, string_id sid) -> resource_handle {
			font_raw*		 raw_ptr   = reinterpret_cast<font_raw*>(raw);
			world_resources& resources = w.get_resources();
			resource_handle	 handle	   = resources.add_resource<font>(sid);
			font&			 res	   = resources.get_resource<font>(handle);
			res.create_from_raw(*raw_ptr, w.get_font_manager(), resources.get_aux());
			delete raw_ptr;
			return handle;
		});

		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<font>(MAX_WORLD_FONTS); });

		m.add_function<void, world&, resource_handle>("destroy"_hs, [](world& w, resource_handle h) -> void {
			world_resources& res = w.get_resources();
			res.get_resource<font>(h).destroy(w.get_font_manager(), res.get_aux());
			res.remove_resource<font>(h);
		});

		m.add_function<void, void*, ostream&>("serialize"_hs, [](void* loader, ostream& stream) -> void {
			font_raw* raw = reinterpret_cast<font_raw*>(loader);
			raw->serialize(stream);
		});
	}

	void font::create_from_raw(const font_raw& raw, vekt::font_manager& fm, chunk_allocator32& alloc)
	{

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		// ffs man.
		unsigned char* data = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(raw.font_data.data()));
		_font				= fm.load_font(data, static_cast<unsigned int>(raw.font_data.size()), static_cast<unsigned int>(raw.point_size), 32, 128, static_cast<vekt::font_type>(raw.font_type), raw.sdf_padding, raw.sdf_edge, raw.sdf_distance);
	}

	void font::destroy(vekt::font_manager& fm, chunk_allocator32& alloc)
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		fm.unload_font(_font);
	}
}
