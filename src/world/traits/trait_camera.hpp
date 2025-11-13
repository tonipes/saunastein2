// Copyright (c) 2025 Inan Evin
#pragma once

#include "world/traits/common_trait.hpp"
#include "reflection/trait_reflection.hpp"
#include "world/world_max_defines.hpp"

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{
	class ostream;
	class istream;
	class world;

	class trait_camera
	{
	public:
		// -----------------------------------------------------------------------------
		// trait
		// -----------------------------------------------------------------------------

		void on_add(world& w);
		void on_remove(world& w);

		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(const nlohmann::json& j, world& w);
#endif

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		void set_values(world& w, float near_plane, float far_plane, float fov_degrees, std::initializer_list<float> cascades = {0.01f, 0.075f, 0.12f, 0.25f});
		void set_main(world& w);

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
		template <typename T, int> friend class trait_cache;

	private:
		trait_header _header = {};

		static_vector<float, MAX_SHADOW_CASCADES> _cascades;
		float									  _near		   = 0.1f;
		float									  _far		   = 0.1f;
		float									  _fov_degrees = 45.0f;
	};

	REGISTER_TRAIT(trait_camera);
}
