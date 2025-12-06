// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "memory/pool_handle.hpp"

namespace SFG
{

	enum class animation_transition_compare : uint8
	{
		equals,
		not_equals,
		greater,
		lesser,
		gequals,
		lequals,
	};

	struct animation_transition
	{
		pool_handle16				 _next_transition = {};
		pool_handle16				 to_state		  = {};
		pool_handle16				 parameter		  = {};
		float						 target_value	  = 0.0f;
		float						 _current_time	  = 0.0f;
		float						 duration		  = 1.0f;
		uint8						 priority		  = 0;
		animation_transition_compare compare		  = animation_transition_compare::greater;
	};
}
