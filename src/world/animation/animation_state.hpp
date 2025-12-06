// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/bitmask.hpp"
#include "math/vector2.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{
	enum animation_state_flags : uint8
	{
		animation_state_flags_is_looping = 1 << 0,
		animation_state_flags_is_1d		 = 1 << 1,
		animation_state_flags_is_2d		 = 1 << 2,
	};

	struct animation_state_sample
	{
		resource_handle animation	 = {};
		pool_handle16	_next_sample = {};
		vector2			blend_point	 = vector2::zero;
	};

	struct animation_state
	{
		pool_handle16  _first_sample		 = {};
		pool_handle16  _first_out_transition = {};
		pool_handle16  _next_state			 = {};
		pool_handle16  mask					 = {};
		pool_handle16  blend_weight_param_x	 = {};
		pool_handle16  blend_weight_param_y	 = {};
		float		   duration				 = 0.0f;
		float		   _current_time		 = 0.0f;
		bitmask<uint8> flags				 = 0;
	};
}
