// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"

namespace SFG
{
	class world_listener
	{
	public:
		world_listener()		  = default;
		virtual ~world_listener() = default;

		virtual void on_started_play()	  = 0;
		virtual void on_stopped_play()	  = 0;
		virtual void on_started_physics() = 0;
		virtual void on_stopped_physics() = 0;
	};
}
