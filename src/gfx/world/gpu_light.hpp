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
		vector4 color = vector4::one;
	};

	struct gpu_point_light
	{
		vector4 color = vector4::one;
	};

	struct gpu_spot_light
	{
		vector4 color = vector4::one;
	};

}
