// Copyright (c) 2025 Inan Evin

#pragma once
#include "gfx/common/gfx_common.hpp"
#include "world/common_world.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{
	class buffer;

	struct renderable_object
	{
		buffer*		vertex_buffer = nullptr;
		buffer*		index_buffer  = nullptr;
		uint32		vertex_start  = 0;
		uint32		index_start	  = 0;
		uint32		index_count	  = 0;
		uint32		gpu_entity	  = 0;
		resource_id material	  = {};
		uint8		is_skinned	  = 0;
	};
}
