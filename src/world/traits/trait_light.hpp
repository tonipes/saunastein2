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

	struct trait_dir_light_reflection
	{
		trait_dir_light_reflection();
	};

	struct trait_point_light_reflection
	{
		trait_point_light_reflection();
	};

	struct trait_spot_light_reflection
	{
		trait_spot_light_reflection();
	};

	class trait_dir_light
	{
	public:
		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(nlohmann::json& j, world& w);
#endif

	private:
		friend class entity_manager;
		friend class trait_dir_light_reflection;

		void on_add(world& w);
		void on_remove(world& w);

	private:
		trait_header _header	 = {};
		color		 _base_color = color::white;
	};

	class trait_spot_light
	{
	public:
		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(nlohmann::json& j, world& w);
#endif

	private:
		friend class entity_manager;
		friend class trait_spot_light_reflection;

		void on_add(world& w);
		void on_remove(world& w);

	private:
		trait_header _header	 = {};
		color		 _base_color = color::white;
	};

	class trait_point_light
	{
	public:
		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(nlohmann::json& j, world& w);
#endif

	private:
		friend class entity_manager;
		friend class trait_point_light_reflection;

		void on_add(world& w);
		void on_remove(world& w);

	private:
		trait_header _header	 = {};
		color		 _base_color = color::white;
	};

	REGISTER_TRAIT(trait_dir_light, trait_types::trait_type_dir_light, trait_dir_light_reflection);
	REGISTER_TRAIT(trait_spot_light, trait_types::trait_type_spot_light, trait_dir_light_reflection);
	REGISTER_TRAIT(trait_point_light, trait_types::trait_type_point_light, trait_dir_light_reflection);
}
