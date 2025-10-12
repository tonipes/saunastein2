// Copyright (c) 2025 Inan Evin

#include "render_events.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{
	void render_event_header::serialize(ostream& stream) const
	{
		stream << index;
		stream << event_type;
	}
	void render_event_header::deserialize(istream& stream)
	{
		stream >> index;
		stream >> event_type;
	}
}
