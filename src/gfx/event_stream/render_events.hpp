// Copyright (c) 2025 Inan Evin

#pragma once

#include "render_event_common.hpp"
#include "resources/common_resources.hpp"
#include "world/world_constants.hpp"

namespace SFG
{

	class ostream;
	class istream;

	struct render_event_header
	{
		world_id		  index = 0;
		render_event_type event_type;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

}
