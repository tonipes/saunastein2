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

	void render_event_ambient::serialize(ostream& stream) const
	{
		stream << entity_index;
		stream << base_color;
	}

	void render_event_ambient::deserialize(istream& stream)
	{
		stream >> entity_index;
		stream >> base_color;
	}

	void render_event_point_light::serialize(ostream& stream) const
	{
		stream << shadow_resolution;
		stream << cast_shadows;
		stream << entity_index;
		stream << base_color;
		stream << range;
		stream << intensity;
	}

	void render_event_point_light::deserialize(istream& stream)
	{
		stream >> shadow_resolution;
		stream >> cast_shadows;
		stream >> entity_index;
		stream >> base_color;
		stream >> range;
		stream >> intensity;
	}

	void render_event_dir_light::serialize(ostream& stream) const
	{
		stream << shadow_resolution;
		stream << cast_shadows;
		stream << entity_index;
		stream << base_color;
		stream << intensity;
	}

	void render_event_dir_light::deserialize(istream& stream)
	{
		stream >> shadow_resolution;
		stream >> cast_shadows;
		stream >> entity_index;
		stream >> base_color;
		stream >> intensity;
	}

	void render_event_spot_light::serialize(ostream& stream) const
	{
		stream << shadow_resolution;
		stream << cast_shadows;
		stream << entity_index;
		stream << base_color;
		stream << range;
		stream << intensity;
		stream << inner_cone;
		stream << outer_cone;
	}

	void render_event_spot_light::deserialize(istream& stream)
	{
		stream >> shadow_resolution;
		stream >> cast_shadows;
		stream >> entity_index;
		stream >> base_color;
		stream >> range;
		stream >> intensity;
		stream >> inner_cone;
		stream >> outer_cone;
	}

}
