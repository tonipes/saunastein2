// Copyright (c) 2025 Inan Evin
#pragma once

#include "world/components/common_comps.hpp"
#include "game/game_max_defines.hpp"
#include "reflection/component_reflection.hpp"
#include "data/static_vector.hpp"
#include "math/color.hpp"
#include "math/vector2ui16.hpp"

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{
#define SFG_LIGHT_CANDELA_MULT 1.0f / 683.0f

	class ostream;
	class istream;
	class world;

	class comp_dir_light
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

		void set_values(world& w, const color& c, float intensity);
		void set_shadow_values(world& w, uint8 cast_shadows, const vector2ui16& resolution);

		inline const color& get_color() const
		{
			return _base_color;
		}

		inline float get_intensity() const
		{
			return _intensity;
		}

	private:
		template <typename T, int> friend class comp_cache;

		void send_event(world& w);

	private:
		component_header _header			= {};
		color			 _base_color		= color::white;
		vector2ui16		 _shadow_resolution = vector2ui16(256, 256);
		float			 _intensity			= 0.0f;
		uint8			 _cast_shadows		= 0;
	};

	class comp_spot_light
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

		void set_values(world& w, const color& c, float range, float intensity, float inner_cone, float outer_cone);
		void set_shadow_values(world& w, uint8 cast_shadows, float near_plane, const vector2ui16& resolution);

		inline const color& get_color() const
		{
			return _base_color;
		}

		inline float get_range() const
		{
			return _range;
		}

		inline float get_intensity() const
		{
			return _intensity;
		}

		inline float get_inner_cone() const
		{
			return _inner_cone;
		}

		inline float get_outer_cone() const
		{
			return _outer_cone;
		}

		inline float get_near_plane() const
		{
			return _near_plane;
		}

	private:
		template <typename T, int> friend class comp_cache;

		void send_event(world& w);

	private:
		component_header _header			= {};
		color			 _base_color		= color::white;
		vector2ui16		 _shadow_resolution = vector2ui16(256, 256);
		float			 _range				= 0.0f;
		float			 _intensity			= 0.0f;
		float			 _inner_cone		= 0.0f;
		float			 _outer_cone		= 0.0f;
		float			 _near_plane		= 0.0f;
		uint8			 _cast_shadows		= 0;
	};

	class comp_point_light
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

		void set_values(world& w, const color& c, float range, float intensity);
		void set_shadow_values(world& w, uint8 cast_shadows, float near_plane, const vector2ui16& resolution);

		inline const color& get_color() const
		{
			return _base_color;
		}

		inline float get_range() const
		{
			return _range;
		}

		inline float get_intensity() const
		{
			return _intensity;
		}

		inline float get_near_plane() const
		{
			return _near_plane;
		}

	private:
		template <typename T, int> friend class comp_cache;

		void send_event(world& w);

	private:
		component_header _header			= {};
		color			 _base_color		= color::white;
		vector2ui16		 _shadow_resolution = vector2ui16(256, 256);
		float			 _range				= 0.0f;
		float			 _intensity			= 0.0f;
		float			 _near_plane		= 0.0f;
		uint8			 _cast_shadows		= 0;
	};

	REGISTER_TRAIT(comp_dir_light);
	REGISTER_TRAIT(comp_spot_light);
	REGISTER_TRAIT(comp_point_light);
}
