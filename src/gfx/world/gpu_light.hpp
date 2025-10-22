// Copyright (c) 2025 Inan Evin

#pragma once
#include "math/vector4.hpp"

namespace SFG
{
#define MAX_GPU_DIR_LIGHTS	 4
#define MAX_GPU_POINT_LIGHTS 32
#define MAX_GPU_SPOT_LIGHTS	 32

	struct gpu_dir_light
	{
		vector4 color	   = vector4::one;
		float	range	   = 0.0f;
		float	intensity  = 0;
		float	padding[3] = {};
	};

	struct gpu_point_light
	{
		vector4 color	   = vector4::one;
		float	range	   = 0.0f;
		float	intensity  = 0;
		float	padding[3] = {};
	};

	struct gpu_spot_light
	{
		vector4 color		= vector4::one;
		float	range		= 0.0f;
		float	intensity	= 0;
		float	innter_cone = 0.0f;
		float	outer_cone	= 0.0f;
		float	padding		= 0.0f;
	};

}
