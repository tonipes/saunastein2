// Copyright (c) 2025 Inan Evin

#pragma once
#include "common/size_definitions.hpp"
#include "math/matrix4x4.hpp"
#include "math/frustum.hpp"
#include "data/static_vector.hpp"
#include "world/world_max_defines.hpp"

namespace SFG
{
	struct view
	{
		frustum									  view_frustum		   = {};
		matrix4x4								  view_matrix		   = matrix4x4::identity;
		matrix4x4								  proj_matrix		   = matrix4x4::identity;
		matrix4x4								  view_proj_matrix	   = matrix4x4::identity;
		matrix4x4								  inv_view_proj_matrix = matrix4x4::identity;
		static_vector<float, MAX_SHADOW_CASCADES> cascades;
		vector3									  position			  = vector3::zero;
		float									  near_plane		  = 0.0f;
		float									  far_plane			  = 0.0f;
		float									  fov_degrees		  = 0.0f;
		uint32									  cascsades_gpu_index = 0;
	};
}
