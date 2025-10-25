// Copyright (c) 2025 Inan Evin

#pragma once

#include "render_proxy_common.hpp"
#include "memory/chunk_handle.hpp"
#include "world/world_constants.hpp"
#include "resources/common_resources.hpp"
#include "math/color.hpp"
#include "math/vector2ui16.hpp"

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
		vector3				base_color = vector3::one;
		world_id			entity	   = 0;
		render_proxy_status status	   = render_proxy_status::rps_inactive;
	};

	struct render_proxy_dir_light
	{
		vector3				base_color	 = vector3::one;
		vector2ui16			shadow_res	 = vector2ui16(256, 256);
		float				intensity	 = 0;
		world_id			entity		 = 0;
		uint8				cast_shadows = 0;
		render_proxy_status status		 = render_proxy_status::rps_inactive;
	};

	struct render_proxy_point_light
	{
		vector3				base_color	 = vector3::one;
		vector2ui16			shadow_res	 = vector2ui16(256, 256);
		float				range		 = 0.0f;
		float				intensity	 = 0;
		world_id			entity		 = 0;
		uint8				cast_shadows = 0;
		render_proxy_status status		 = render_proxy_status::rps_inactive;
	};

	struct render_proxy_spot_light
	{
		vector3				base_color	 = vector3::one;
		vector2ui16			shadow_res	 = vector2ui16(256, 256);
		float				range		 = 0.0f;
		float				intensity	 = 0;
		float				inner_cone	 = 0.0f;
		float				outer_cone	 = 0.0f;
		world_id			entity		 = 0;
		uint8				cast_shadows = 0;
		render_proxy_status status		 = render_proxy_status::rps_inactive;
	};
}
