// Copyright (c) 2025 Inan Evin
#pragma once

#include "world/traits/common_trait.hpp"
#include "reflection/trait_reflection.hpp"
#include "math/color.hpp"

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{
	class ostream;
	class istream;
	class world;

	class trait_dir_light
	{
	public:
		void on_add(world& w);
		void on_remove(world& w);
		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

		void set_values(world& w, const color& c, float range, float intensity);

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(const nlohmann::json& j, world& w);
#endif

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

	private:
		template <typename T, int> friend class trait_cache;

		void send_event(world& w);

	private:
		trait_header _header	 = {};
		color		 _base_color = color::white;
		float		 _range		 = 0.0f;
		float		 _intensity	 = 0.0f;
	};

	class trait_spot_light
	{
	public:
		void on_add(world& w);
		void on_remove(world& w);
		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);
		void set_values(world& w, const color& c, float range, float intensity, float inner_cone, float outer_cone);

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(const nlohmann::json& j, world& w);
#endif

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

	private:
		template <typename T, int> friend class trait_cache;

		void send_event(world& w);

	private:
		trait_header _header	 = {};
		color		 _base_color = color::white;
		float		 _range		 = 0.0f;
		float		 _intensity	 = 0.0f;
		float		 _inner_cone = 0.0f;
		float		 _outer_cone = 0.0f;
	};

	class trait_point_light
	{
	public:
		void on_add(world& w);
		void on_remove(world& w);
		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);
		void set_values(world& w, const color& c, float range, float intensity);

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(const nlohmann::json& j, world& w);
#endif

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

	private:
		template <typename T, int> friend class trait_cache;

		void send_event(world& w);

	private:
		trait_header _header	 = {};
		color		 _base_color = color::white;
		float		 _range		 = 0.0f;
		float		 _intensity	 = 0.0f;
	};

	REGISTER_TRAIT(trait_dir_light);
	REGISTER_TRAIT(trait_spot_light);
	REGISTER_TRAIT(trait_point_light);
}
