// Copyright (c) 2025 Inan Evin

#include "font.hpp"
#include "font_raw.hpp"
#include "gui/vekt.hpp"
#include "world/world.hpp"

#ifdef SFG_TOOLMODE
#include "project/engine_data.hpp"
#endif

namespace SFG
{
	void font::create_from_loader(const font_raw& raw, world& w, resource_handle handle)
	{
		resource_manager&	rm	  = w.get_resource_manager();
		chunk_allocator32&	alloc = rm.get_aux();
		vekt::font_manager& fm	  = w.get_font_manager();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		// ffs man.
		unsigned char* data = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(raw.font_data.data()));
		_font				= fm.load_font(data, static_cast<unsigned int>(raw.font_data.size()), static_cast<unsigned int>(raw.point_size), 32, 128, static_cast<vekt::font_type>(raw.font_type), raw.sdf_padding, raw.sdf_edge, raw.sdf_distance);
	}

	void font::destroy(world& w, resource_handle handle)
	{
		resource_manager&	rm	  = w.get_resource_manager();
		chunk_allocator32&	alloc = rm.get_aux();
		vekt::font_manager& fm	  = w.get_font_manager();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		fm.unload_font(_font);
	}
}
