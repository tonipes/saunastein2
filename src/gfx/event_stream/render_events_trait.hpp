// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/vector.hpp"
#include "world/world_constants.hpp"

namespace SFG
{

	class ostream;
	class istream;

	struct render_event_model_instance
	{
		world_id	   entity_index = 0;
		vector<uint16> materials;
		uint16		   model	   = 0;
		uint16		   mesh		   = 0;
		uint8		   single_mesh = 0;

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

}
