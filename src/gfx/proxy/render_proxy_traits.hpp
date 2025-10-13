// Copyright (c) 2025 Inan Evin

#pragma once

#include "render_proxy_common.hpp"
#include "memory/chunk_handle.hpp"
#include "world/world_constants.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{
	struct render_proxy_model_instance
	{
		chunk_handle32		materials	   = {};
		world_id			entity		   = 0;
		resource_id			model		   = 0;
		resource_id			material_count = 0;
		resource_id			mesh		   = 0;
		uint8				single_mesh	   = 0;
		render_proxy_status status		   = render_proxy_status::rps_inactive;
	};

	struct render_proxy_camera
	{
		world_id			entity		= 0;
		float				near_plane	= 0.0f;
		float				far_plane	= 0.0f;
		float				fov_degrees = 0.0f;
		render_proxy_status status		= render_proxy_status::rps_inactive;
	};
}
