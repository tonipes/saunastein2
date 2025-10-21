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
		entity_flags_local_transform_dirty = 1 << 0,
		entity_flags_abs_transform_dirty   = 1 << 1,
		entity_flags_abs_rotation_dirty	   = 1 << 2,
		entity_flags_render_proxy_dirty	   = 1 << 3,
		entity_flags_invisible			   = 1 << 4,
		entity_flags_prev_transform_init   = 1 << 5,
	};

	struct entity_meta
	{
		const char*		name			   = "";
		bitmask<uint16> flags			   = entity_flags_local_transform_dirty | entity_flags_abs_transform_dirty;
		uint8			render_proxy_count = 0;
	};

	struct entity_family
	{
		world_handle parent		  = {};
		world_handle first_child  = {};
		world_handle prev_sibling = {};
		world_handle next_sibling = {};
	};

	struct entity_trait
	{
		string_id	 trait_type	  = 0;
		world_handle trait_handle = {};

		bool operator==(const entity_trait& other) const
		{
			return trait_type == other.trait_type && trait_handle == other.trait_handle;
		}
	};

#define MAX_TRAITS_PER_ENTITY 8

	struct entity_trait_register
	{
		static_vector<entity_trait, MAX_TRAITS_PER_ENTITY> traits;
	};
}
