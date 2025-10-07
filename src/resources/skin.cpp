// Copyright (c) 2025 Inan Evin

#include "skin.hpp"
#include "skin_raw.hpp"
#include "memory/chunk_allocator.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"

namespace SFG
{
	skin_reflection g_skin_reflection;

	skin_reflection::skin_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<skin>::value, type_id<skin>::index, "");
		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<skin>(MAX_WORLD_SKINS); });
	}

	void skin::create_from_raw(const skin_raw& raw, chunk_allocator32& alloc)
	{
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

	void skin::destroy(chunk_allocator32& alloc)
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		alloc.free(_joints);

		_joints = {};
	}

}