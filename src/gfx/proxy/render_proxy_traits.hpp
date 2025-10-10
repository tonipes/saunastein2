// Copyright (c) 2025 Inan Evin

#pragma once

#include "render_proxy_common.hpp"

namespace SFG
{
	struct render_proxy_model_instance
	{
		render_proxy_status status		   = render_proxy_status::rps_inactive;
		uint32				entity		   = 0;
		uint16				model		   = 0;
		uint16				materials	   = 0;
		uint16				material_count = 0;
		uint16				mesh		   = 0;
		uint8				single_mesh	   = 0;
	};
}
