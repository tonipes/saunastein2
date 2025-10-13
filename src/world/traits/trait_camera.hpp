// Copyright (c) 2025 Inan Evin
#pragma once

#include "world/traits/common_trait.hpp"
#include "reflection/trait_reflection.hpp"

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{
	class ostream;
	class istream;
	class world;
	class world_resources;
	class render_event_stream;

	struct trait_camera_reflection
	{
		trait_camera_reflection();
	};

	class trait_camera
	{
	public:
		void set_values(world& w, float near_plane, float far_plane, float fov_degrees);
		void set_main(world& w);

		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(nlohmann::json& j, world& w);
#endif

		inline float get_near() const
		{
			return _near;
		}

		inline float get_far() const
		{
			return _far;
		}

		inline float get_fov_degrees() const
		{
			return _fov_degrees;
		}

	private:
		friend class entity_manager;

		void on_add(world& w);
		void on_remove(world& w);

	private:
		trait_header _header = {};

		float _near		   = 0.1f;
		float _far		   = 0.1f;
		float _fov_degrees = 90.0f;
	};

	REGISTER_TRAIT(trait_camera, trait_types::trait_type_camera, trait_camera_reflection);
}
