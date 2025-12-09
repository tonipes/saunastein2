// Copyright (c) 2025 Inan Evin

#include "render_events_entity.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{
	void render_event_entity_transform::serialize(ostream& stream) const
	{
		stream << abs_model;
		stream << position;
		stream << rotation;
	}

	void render_event_entity_transform::deserialize(istream& stream)
	{
		stream >> abs_model;
		stream >> position;
		stream >> rotation;
	}

	void render_event_entity_visibility::serialize(ostream& stream) const
	{
		stream << visible;
	}

	void render_event_entity_visibility::deserialize(istream& stream)
	{
		stream >> visible;
	}
}
