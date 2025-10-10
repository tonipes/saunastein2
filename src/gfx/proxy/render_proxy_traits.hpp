// Copyright (c) 2025 Inan Evin

#pragma once

#include "render_proxy_common.hpp"
#include "memory/chunk_handle.hpp"

namespace SFG
{
	struct render_proxy_model_instance
	{
		chunk_handle32		materials	   = {};
		uint32				entity		   = 0;
		uint16				model		   = 0;
		uint16				material_count = 0;
		uint16				mesh		   = 0;
		uint8				single_mesh	   = 0;
		render_proxy_status status		   = render_proxy_status::rps_inactive;
	};
}
