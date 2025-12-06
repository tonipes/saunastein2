// Copyright (c) 2025 Inan Evin

#pragma once

#include "memory/pool_handle.hpp"
#include "memory/chunk_handle.hpp"

namespace SFG
{
	struct animation_state_machine
	{
		pool_handle16  active_state			= {};
		pool_handle16  _first_state			= {};
		pool_handle16  _first_parameter		= {};
		pool_handle16  _active_transition	= {};
		chunk_handle32 joint_entities		= {};
		uint16		   joint_entities_count = 0;
	};
}
