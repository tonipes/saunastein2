// Copyright (c) 2025 Inan Evin

#pragma once

#include "world/world_constants.hpp"
#include "data/vector.hpp"
#include "math/vector3.hpp"

namespace SFG
{
	struct ray_result
	{
		world_handle hit_entity	   = {};
		vector3		 hit_point	   = vector3::zero;
		float		 hit_distance = 0.0f;
	};
}
