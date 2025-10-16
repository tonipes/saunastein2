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

	struct trait_light_reflection
	{
		trait_light_reflection();
	};

	enum class light_type : uint8
	{
		light_type_directional = 0,
		light_type_point,
		light_type_spot,
	};

	class trait_light
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
		friend class trait_light_reflection;

		void on_add(world& w);
		void on_remove(world& w);

	private:
		trait_header _header = {};
	};

	REGISTER_TRAIT(trait_light, trait_types::trait_type_light, trait_light_reflection);
}
