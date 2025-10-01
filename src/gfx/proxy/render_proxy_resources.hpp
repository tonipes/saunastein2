// Copyright (c) 2025 Inan Evin

#pragma once

#include "gfx/common/gfx_common.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{
	struct render_proxy_texture
	{
		resource_handle handle		 = {};
		gfx_id			hw			 = 0;
		gfx_id			intermediate = 0;
	};

}
