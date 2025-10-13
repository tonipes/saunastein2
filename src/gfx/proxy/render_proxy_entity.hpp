// Copyright (c) 2025 Inan Evin

#pragma once
#include "math/matrix4x3.hpp"
#include "data/bitmask.hpp"
#include "render_proxy_common.hpp"
#include "world/world_constants.hpp"

namespace SFG
{

	enum render_proxy_entity_flags : uint8
	{
		render_proxy_entity_invisible = 1 << 0,
	};

	struct render_proxy_entity
	{
		matrix4x3			model  = {};
		world_id			handle = 0;
		uint32				padding[2];
		render_proxy_status status = render_proxy_status::rps_inactive;
		bitmask<uint8>		flags  = 0;
	};
}
