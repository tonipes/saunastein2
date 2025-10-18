// Copyright (c) 2025 Inan Evin

#pragma once
#include "gpu_bone.hpp"
#include "gpu_entity.hpp"
#include "gpu_light.hpp"
#include "renderable.hpp"
#include "math/matrix4x4.hpp"
#include "math/frustum.hpp"

namespace SFG
{
#define MAX_RENDERABLES 1024

	struct view
	{
		matrix4x4 view_matrix	   = matrix4x4::identity;
		matrix4x4 proj_matrix	   = matrix4x4::identity;
		matrix4x4 view_proj_matrix = matrix4x4::identity;
		frustum	  view_frustum	   = {};
	};

	struct world_render_data
	{
		static_vector<renderable_object, MAX_RENDERABLES>	 objects;
		static_vector<gpu_entity, MAX_GPU_ENTITIES>			 entities;
		static_vector<gpu_dir_light, MAX_GPU_DIR_LIGHTS>	 dir_lights;
		static_vector<gpu_point_light, MAX_GPU_POINT_LIGHTS> point_lights;
		static_vector<gpu_spot_light, MAX_GPU_SPOT_LIGHTS>	 spot_lights;
		static_vector<gpu_bone, MAX_GPU_BONES>				 bones;

		inline void reset()
		{
			objects.clear();
			entities.clear();
		}
	};
}
