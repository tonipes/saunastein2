// Copyright (c) 2025 Inan Evin

#pragma once
#include "math/vector3.hpp"
#include "common/size_definitions.hpp"

namespace SFG
{
	struct gpu_dir_light
	{
		uint32	entity_index = 0;
		vector3 color		 = vector3::one;
		float	intensity	 = 0;
		float	padding[3]	 = {};
	};

	struct gpu_point_light
	{
		uint32	entity_index = 0;
		vector3 color		 = vector3::one;
		float	range		 = 0.0f;
		float	intensity	 = 0;
		float	padding[2]	 = {};
	};

	struct gpu_spot_light
	{
		uint32	entity_index = 0;
		vector3 color		 = vector3::one;
		float	range		 = 0.0f;
		float	intensity	 = 0;
		float	inner_cone	 = 0.0f;
		float	outer_cone	 = 0.0f;
	};

}
