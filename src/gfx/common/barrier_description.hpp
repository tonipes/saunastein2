// Copyright (c) 2025 Inan Evin
#pragma once

#include "data/bitmask.hpp"
#include "gfx/common/gfx_constants.hpp"

namespace SFG
{
	enum barrier_flags
	{
		baf_is_resource	 = 1 << 0,
		baf_is_texture	 = 1 << 1,
		baf_is_swapchain = 1 << 2,
	};

	enum resource_state : uint32
	{
		resource_state_common		   = 1 << 0,
		resource_state_vertex_cbv	   = 1 << 1,
		resource_state_index_buffer	   = 1 << 2,
		resource_state_render_target   = 1 << 3,
		resource_state_uav			   = 1 << 4,
		resource_state_depth_write	   = 1 << 5,
		resource_state_depth_read	   = 1 << 6,
		resource_state_non_ps_resource = 1 << 7,
		resource_state_ps_resource	   = 1 << 8,
		resource_state_indirect_arg	   = 1 << 9,
		resource_state_copy_dest	   = 1 << 10,
		resource_state_copy_source	   = 1 << 11,
		resource_state_resolve_dest	   = 1 << 12,
		resource_state_resolve_source  = 1 << 13,
		resource_state_generic_read	   = 1 << 14,
		resource_state_present		   = 1 << 15,
	};

	struct barrier
	{
		gfx_id		   resource	   = 0;
		bitmask<uint8> flags	   = 0;
		uint32		   from_states = 0;
		uint32		   to_states   = 0;
	};

}