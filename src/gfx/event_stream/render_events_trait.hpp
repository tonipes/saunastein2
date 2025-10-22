// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/vector.hpp"
#include "world/world_constants.hpp"
#include "math/color.hpp"

namespace SFG
{

	class ostream;
	class istream;

	struct render_event_mesh_instance
	{
		world_id entity_index = 0;
		uint16	 model		  = 0;
		uint16	 mesh		  = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_camera
	{
		world_id entity_index = 0;
		float	 near_plane	  = 0.0f;
		float	 far_plane	  = 0.0f;
		float	 fov_degrees  = 0.0f;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_ambient
	{
		world_id entity_index = 0;
		color	 base_color	  = color::white;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_point_light
	{
		world_id entity_index = 0;
		color	 base_color	  = color::white;
		float	 range		  = 0.0f;
		float	 intensity	  = 0.0f;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_dir_light
	{
		world_id entity_index = 0;
		color	 base_color	  = color::white;
		float	 range		  = 0.0f;
		float	 intensity	  = 0.0f;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_spot_light
	{
		world_id entity_index = 0;
		color	 base_color	  = color::white;
		float	 range		  = 0.0f;
		float	 intensity	  = 0.0f;
		float	 inner_cone	  = 0.0f;
		float	 outer_cone	  = 0.0f;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};
}
