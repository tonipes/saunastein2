// Copyright (c) 2025 Inan Evin

#pragma once

#include "render_proxy_common.hpp"
#include "memory/chunk_handle.hpp"
#include "world/world_constants.hpp"
#include "resources/common_resources.hpp"
#include "math/color.hpp"

namespace SFG
{
	struct render_proxy_mesh_instance
	{
		world_id			entity = 0;
		resource_id			model  = 0;
		resource_id			mesh   = 0;
		render_proxy_status status = render_proxy_status::rps_inactive;
	};

	struct render_proxy_camera
	{
		world_id			entity		= 0;
		float				near_plane	= 0.0f;
		float				far_plane	= 0.0f;
		float				fov_degrees = 0.0f;
		render_proxy_status status		= render_proxy_status::rps_inactive;
	};

	struct render_proxy_ambient
	{
		world_id			entity		  = 0;
		color				ambient_color = color::white;
		render_proxy_status status		  = render_proxy_status::rps_inactive;
	};

	struct render_proxy_dir_light
	{
		world_id			entity	   = 0;
		color				base_color = color::white;
		render_proxy_status status	   = render_proxy_status::rps_inactive;
	};

	struct render_proxy_point_light
	{
		world_id			entity	   = 0;
		color				base_color = color::white;
		render_proxy_status status	   = render_proxy_status::rps_inactive;
	};

	struct render_proxy_spot_light
	{
		world_id			entity	   = 0;
		color				base_color = color::white;
		render_proxy_status status	   = render_proxy_status::rps_inactive;
	};
}
