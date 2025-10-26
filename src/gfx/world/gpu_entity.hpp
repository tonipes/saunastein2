// Copyright (c) 2025 Inan Evin

#pragma once
#include "math/matrix4x4.hpp"
#include "math/matrix4x3.hpp"
#include "math/matrix3x3.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"

namespace SFG
{

	class buffer;

	struct gpu_entity
	{
		matrix4x4 model	   = matrix4x4::identity;
		matrix4x4 normal   = matrix4x4::identity;
		vector4	  position = vector4::zero;
		vector4	  rotation = vector4::zero;
		vector4	  forward  = vector4::zero;
		vector4	  padding  = vector4::zero;
	};
}
