// Copyright (c) 2025 Inan Evin

#pragma once
#include "common/size_definitions.hpp"
#include "world/common_entity.hpp"
#include "data/bitmask.hpp"
#include "world/world_constants.hpp"

namespace SFG
{
	enum component_flags
	{
		component_flags_is_disabled = 1 << 0,
	};

	struct component_header
	{
		world_handle   entity;
		world_handle   own_handle;
		bitmask<uint8> flags;
	};
}
