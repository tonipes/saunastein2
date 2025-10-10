// Copyright (c) 2025 Inan Evin

#pragma once
#include "math/matrix4x3.hpp"
#include "render_proxy_common.hpp"

namespace SFG
{
	struct render_proxy_entity
	{
		matrix4x3			model  = {};
		uint32				handle = 0;
		uint32				padding[2];
		render_proxy_status status = render_proxy_status::rps_inactive;
	};
}
