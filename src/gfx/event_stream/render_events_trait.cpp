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
		stream << skin;
		stream << skin_node_entities;
	}

	void render_event_mesh_instance::deserialize(istream& stream)
	{
		stream >> entity_index;
		stream >> model;
		stream >> mesh;
		stream >> skin;
		stream >> skin_node_entities;
	}

	void render_event_camera::serialize(ostream& stream) const
	{
		stream << entity_index;
		stream << near_plane;
		stream << far_plane;
		stream << fov_degrees;
		stream << static_cast<uint32>(cascades.size());
		for (float f : cascades)
			stream << f;
	}

	void render_event_camera::deserialize(istream& stream)
	{
		stream >> entity_index;
		stream >> near_plane;
		stream >> far_plane;
		stream >> fov_degrees;
		uint32 cascade_count = 0;
		stream >> cascade_count;
		for (uint32 i = 0; i < cascade_count; i++)
		{
			float f = 0.0f;
			stream >> f;
			cascades.push_back(f);
		}
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

	void render_event_canvas::serialize(ostream& stream) const
	{
		stream << entity_index;
		stream << is_3d;
		stream << vertex_size;
		stream << index_size;
	}

	void render_event_canvas::deserialize(istream& stream)
	{
		stream >> entity_index;
		stream >> is_3d;
		stream >> vertex_size;
		stream >> index_size;
	}

	void render_event_canvas_add_draw::serialize(ostream& stream) const
	{
		stream << clip;
		stream << start_index;
		stream << start_vertex;
		stream << index_count;
		stream << material_handle;
		stream << atlas_handle;
		stream << atlas_exists;
		stream << vertex_data_size;
		stream << index_data_size;

		stream.write_raw(vertex_data, vertex_data_size);
		stream.write_raw(index_data, index_data_size);
	}

	void render_event_canvas_add_draw::deserialize(istream& stream)
	{
		stream >> clip;
		stream >> start_index;
		stream >> start_vertex;
		stream >> index_count;
		stream >> material_handle;
		stream >> atlas_handle;
		stream >> atlas_exists;
		stream >> vertex_data_size;
		stream >> index_data_size;

		vertex_data = stream.get_data_current();
		stream.skip_by(vertex_data_size);
		index_data = stream.get_data_current();
		stream.skip_by(index_data_size);
	}

}
