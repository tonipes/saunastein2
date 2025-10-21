// Copyright (c) 2025 Inan Evin

#include "skin.hpp"
#include "skin_raw.hpp"
#include "memory/chunk_allocator.hpp"
#include "world/world.hpp"

namespace SFG
{

	void skin::create_from_loader(const skin_raw& raw, world& w, resource_handle handle)
	{
		resource_manager&  rm	 = w.get_resource_manager();
		chunk_allocator32& alloc = rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		_root		  = raw.root_joint;
		_joints_count = static_cast<uint16>(raw.joints.size());
		_joints		  = alloc.allocate<skin_joint>(raw.joints.size());

		skin_joint*	 ptr   = reinterpret_cast<skin_joint*>(alloc.get(_joints.head));
		const uint32 count = static_cast<uint32>(raw.joints.size());

		for (uint32 i = 0; i < count; i++)
			ptr[i] = raw.joints[i];
	}

	void skin::destroy(world& w, resource_handle handle)
	{
		resource_manager&  rm	 = w.get_resource_manager();
		chunk_allocator32& alloc = rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		alloc.free(_joints);

		_joints = {};
	}

}