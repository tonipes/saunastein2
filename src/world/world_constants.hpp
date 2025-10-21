// Copyright (c) 2025 Inan Evin

#pragma once
#include "common/size_definitions.hpp"
#include "memory/pool_handle.hpp"

#undef min
#undef max
#include <limits>

namespace SFG
{
	typedef uint32		  world_id;
	typedef pool_handle32 world_handle;

#define NULL_WORLD_ID std::numeric_limits<world_id>::max()

}
