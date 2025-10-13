// Copyright (c) 2025 Inan Evin

#include "render_events_trait.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{
	void render_event_mesh_instance::serialize(ostream& stream) const
	{
		stream << entity_index;
		stream << model;
		stream << mesh;
	}

	void render_event_mesh_instance::deserialize(istream& stream)
	{
		stream >> entity_index;
		stream >> model;
		stream >> mesh;
	}

	void render_event_camera::serialize(ostream& stream) const
	{
		stream << entity_index;
		stream << near_plane;
		stream << far_plane;
		stream << fov_degrees;
	}

	void render_event_camera::deserialize(istream& stream)
	{
		stream >> entity_index;
		stream >> near_plane;
		stream >> far_plane;
		stream >> fov_degrees;
	}

}
