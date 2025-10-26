// Copyright (c) 2025 Inan Evin

#pragma once
#include "math/matrix4x4.hpp"
#include "math/frustum.hpp"

namespace SFG
{

	struct view
	{
		matrix4x4 view_matrix		   = matrix4x4::identity;
		matrix4x4 proj_matrix		   = matrix4x4::identity;
		matrix4x4 view_proj_matrix	   = matrix4x4::identity;
		matrix4x4 inv_view_proj_matrix = matrix4x4::identity;
		frustum	  view_frustum		   = {};
		vector3	  position			   = vector3::zero;
	};
}
