// Copyright (c) 2025 Inan Evin

#pragma once
#include "common/size_definitions.hpp"
#include "data/bitmask.hpp"
#include "common/string_id.hpp"
#include "data/static_vector.hpp"
#include "world/world_constants.hpp"

#undef max

namespace SFG
{
	enum entity_flags : uint8
	{
		entity_flags_invisible			  = 1 << 0,
		entity_flags_abs_transforms_dirty = 1 << 1,
		entity_flags_is_render_proxy	  = 1 << 2,
	};

	struct entity_meta
	{
		const char*		name			   = "";
		uint8			render_proxy_count = 0;
	};

	struct entity_family
	{
		world_handle parent		  = {};
		world_handle first_child  = {};
		world_handle prev_sibling = {};
		world_handle next_sibling = {};
	};

	struct entity_comp
	{
		string_id	 comp_type	 = 0;
		world_handle comp_handle = {};

		bool operator==(const entity_comp& other) const
		{
			return comp_type == other.comp_type && comp_handle == other.comp_handle;
		}
	};

#define MAX_COMPS_PER_ENTITY 8

	struct entity_comp_register
	{
		static_vector<entity_comp, MAX_COMPS_PER_ENTITY> comps;
	};
}
