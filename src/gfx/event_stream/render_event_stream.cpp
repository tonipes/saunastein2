// Copyright (c) 2025 Inan Evin

#include "render_event_stream.hpp"

namespace SFG
{
	void render_event_stream::add_event(const render_event& ev)
	{
		_events.enqueue(ev);
	}
}