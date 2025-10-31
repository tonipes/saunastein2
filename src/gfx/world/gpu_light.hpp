// Copyright (c) 2025 Inan Evin

#pragma once
#include "math/vector4.hpp"
#include "common/size_definitions.hpp"
#include "gfx/common/gfx_constants.hpp"

namespace SFG
{
	struct gpu_dir_light
	{
		vector4 color_entity_index			  = vector4::zero;
		vector4 intensity					  = vector4::zero;
		vector4 shadow_res_map_and_data_index = vector4::zero;
	};

	struct gpu_point_light
	{
		vector4 color_entity_index			  = vector4::zero;
		vector4 intensity_range				  = vector4::zero;
		vector4 shadow_res_map_and_data_index = vector4::zero;
	};

	struct gpu_spot_light
	{
		vector4 color_entity_index			  = vector4::zero;
		vector4 intensity_range_inner_outer	  = vector4::zero;
		vector4 shadow_res_map_and_data_index = vector4::zero;
	};

}
