// Copyright (c) 2025 Inan Evin

#include "render_events_trait.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{
	void render_event_model_instance::serialize(ostream& stream) const
	{
		stream << model;
		stream << mesh;
		stream << single_mesh;
		stream << materials;
	}

	void render_event_model_instance::deserialize(istream& stream)
	{
		stream >> model;
		stream >> mesh;
		stream >> single_mesh;
		stream >> materials;
	}
}
