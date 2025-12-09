// Copyright (c) 2025 Inan Evin

#pragma once
#include "math/matrix4x3.hpp"
#include "math/matrix3x3.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
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
		matrix4x3			model			= {};
		matrix3x3			normal			= {};
		quat				rotation		= quat::identity;
		vector3				position		= vector3::zero;
		uint32				_assigned_index = 0;
		world_id			handle			= 0;
		render_proxy_status status			= render_proxy_status::rps_inactive;
		bitmask<uint8>		flags			= 0;
	};
}
