// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/vector.hpp"
#include "data/static_vector.hpp"
#include "world/world_constants.hpp"
#include "math/vector3.hpp"
#include "math/vector2ui16.hpp"
#include "world/world_max_defines.hpp"

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
		static_vector<float, MAX_SHADOW_CASCADES> cascades;
		world_id								  entity_index = 0;
		float									  near_plane   = 0.0f;
		float									  far_plane	   = 0.0f;
		float									  fov_degrees  = 0.0f;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_ambient
	{
		vector3	 base_color	  = vector3::one;
		world_id entity_index = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_point_light
	{
		vector3		base_color		  = vector3::one;
		vector2ui16 shadow_resolution = vector2ui16(256, 256);
		world_id	entity_index	  = 0;
		float		range			  = 0.0f;
		float		intensity		  = 0.0f;
		uint8		cast_shadows	  = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_dir_light
	{
		vector3		base_color		  = vector3::one;
		vector2ui16 shadow_resolution = vector2ui16(256, 256);
		world_id	entity_index	  = 0;
		float		intensity		  = 0.0f;
		uint8		cast_shadows	  = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_spot_light
	{
		vector3		base_color		  = vector3::one;
		vector2ui16 shadow_resolution = vector2ui16(256, 256);
		world_id	entity_index	  = 0;
		float		range			  = 0.0f;
		float		intensity		  = 0.0f;
		float		inner_cone		  = 0.0f;
		float		outer_cone		  = 0.0f;
		uint8		cast_shadows	  = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};
}
