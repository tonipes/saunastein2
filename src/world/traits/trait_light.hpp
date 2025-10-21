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

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(const nlohmann::json& j, world& w);
#endif

	private:
		template <typename T, int> friend class trait_cache;

	private:
		trait_header _header	 = {};
		color		 _base_color = color::white;
	};

	class trait_spot_light
	{
	public:
		void on_add(world& w);
		void on_remove(world& w);
		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(const nlohmann::json& j, world& w);
#endif

	private:
		template <typename T, int> friend class trait_cache;

	private:
		trait_header _header	 = {};
		color		 _base_color = color::white;
	};

	class trait_point_light
	{
	public:
		void on_add(world& w);
		void on_remove(world& w);
		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(const nlohmann::json& j, world& w);
#endif

	private:
		template <typename T, int> friend class trait_cache;

	private:
		trait_header _header	 = {};
		color		 _base_color = color::white;
	};

	REGISTER_TRAIT(trait_dir_light);
	REGISTER_TRAIT(trait_spot_light);
	REGISTER_TRAIT(trait_point_light);
}
