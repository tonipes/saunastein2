// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "gfx/common/gfx_constants.hpp"

namespace SFG
{
	struct semaphore_data
	{
		gfx_id semaphore = 0;
		uint64 value	 = 0;
	};
}