// Copyright (c) 2025 Inan Evin

#include "render_events_entity.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{
	void render_event_entity::serialize(ostream& stream) const
	{
		stream << abs_model;
	}

	void render_event_entity::deserialize(istream& stream)
	{
		stream >> abs_model;
	}
}
