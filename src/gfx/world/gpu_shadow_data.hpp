// Copyright (c) 2025 Inan Evin

#pragma once
#include "math/matrix4x4.hpp"

namespace SFG
{
	struct gpu_shadow_data
	{
		matrix4x4 light_space_matrix = matrix4x4::identity;
		float	  texel_world		 = 0.0f;
	};
}
