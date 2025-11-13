// Copyright (c) 2025 Inan Evin

#pragma once

#include "world/world_constants.hpp"

namespace SFG
{
	class vector3;

	class physics_contact_listener
	{
	public:
		virtual void on_contact_begin(world_handle e1, world_handle e2, const vector3& p1, const vector3& p2) = 0;
		virtual void on_contact(world_handle e1, world_handle e2, const vector3& p1, const vector3& p2)		  = 0;
		virtual void on_contact_end(world_handle e1, world_handle e2)										  = 0;
	};

}
