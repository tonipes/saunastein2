// Copyright (c) 2025 Inan Evin

#pragma once

#include "render_event_common.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{

	struct render_event_header
	{
		resource_handle	  handle;
		render_event_type event_type;
	};

	struct render_event
	{
		static constexpr size_t MAX_SIZE = 256;
		render_event_header		header;
		uint8					data[MAX_SIZE - sizeof(render_event_header)];
	};
}
