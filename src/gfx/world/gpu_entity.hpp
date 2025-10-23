// Copyright (c) 2025 Inan Evin

#pragma once
#include "math/matrix4x3.hpp"
#include "math/matrix3x3.hpp"
#include "math/vector3.hpp"

namespace SFG
{
#define MAX_GPU_ENTITIES 512

	struct gpu_entity
	{
		matrix4x3 model	   = matrix4x3::identity;
		matrix3x3 normal   = matrix3x3::identity;
		vector3	  position = vector3::zero;
	};
}
