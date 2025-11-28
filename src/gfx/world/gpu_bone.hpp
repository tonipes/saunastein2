// Copyright (c) 2025 Inan Evin

#pragma once
#include "math/matrix4x4.hpp"

namespace SFG
{
	struct gpu_bone
	{
		matrix4x4 mat = matrix4x4::identity;
	};
}
