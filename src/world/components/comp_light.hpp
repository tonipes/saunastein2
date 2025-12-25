/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
		void set_shadow_values(world& w, uint8 cast_shadows, uint8 max_cascades, const vector2ui16& resolution);

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
		uint8			 _max_cascades		= 0;
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
