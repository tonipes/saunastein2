// Copyright (c) 2025 Inan Evin

#include "physical_material.hpp"
#include "physical_material_raw.hpp"
#include "world/world.hpp"

#ifdef SFG_TOOLMODE
#include "project/engine_data.hpp"
#endif
namespace SFG
{
	physical_material::~physical_material()
	{
		SFG_ASSERT(!_flags.is_set(physical_material::flags::created));
	}

	void physical_material::create_from_loader(const physical_material_raw& raw, world& w, resource_handle handle)
	{
		SFG_ASSERT(!_flags.is_set(physical_material::flags::created));
		_flags.set(physical_material::flags::created);

		resource_manager&  rm	 = w.get_resource_manager();
		chunk_allocator32& alloc = rm.get_aux();
#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif
		_settings = raw.settings;
	}

	void physical_material::destroy(world& w, resource_handle handle)
	{
		if (!_flags.is_set(physical_material::flags::created))
			return;

		_flags.remove(physical_material::flags::created);

		resource_manager&  rm	 = w.get_resource_manager();
		chunk_allocator32& alloc = rm.get_aux();
#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif
	}
}
