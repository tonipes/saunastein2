// Copyright (c) 2025 Inan Evin

#pragma once
#include "common/size_definitions.hpp"
#include "data/bitmask.hpp"
#include "world/world_constants.hpp"
#undef max

namespace SFG
{

	enum entity_flags : uint8
	{
		entity_flags_local_transform_dirty = 1 << 0,
		entity_flags_abs_transform_dirty   = 1 << 1,
		entity_flags_abs_rotation_dirty	   = 1 << 2,
	};

	struct entity_meta
	{
		const char*		name  = "";
		bitmask<uint16> flags = entity_flags_local_transform_dirty | entity_flags_abs_transform_dirty;
	};

	struct entity_family
	{
		entity_handle parent	   = {};
		entity_handle first_child  = {};
		entity_handle prev_sibling = {};
		entity_handle next_sibling = {};
	};
}
