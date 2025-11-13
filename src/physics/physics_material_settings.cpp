// Copyright (c) 2025 Inan Evin

#include "physics_material_settings.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
	void physical_material_settings::serialize(ostream& stream) const
	{
		stream << restitution;
		stream << friction;
		stream << angular_damp;
		stream << linear_damp;
		stream << mass;
		stream << gravity_multiplier;
	}

	void physical_material_settings::deserialize(istream& stream)
	{
		stream >> restitution;
		stream >> friction;
		stream >> angular_damp;
		stream >> linear_damp;
		stream >> mass;
		stream >> gravity_multiplier;
	}

#ifdef SFG_TOOLMODE
	void to_json(nlohmann::json& j, const physical_material_settings& s)
	{
		j["restitution"]		= s.restitution;
		j["friction"]			= s.friction;
		j["angular_damp"]		= s.angular_damp;
		j["linear_damp"]		= s.linear_damp;
		j["mass"]				= s.mass;
		j["gravity_multiplier"] = s.gravity_multiplier;
	}

	void from_json(const nlohmann::json& j, physical_material_settings& s)
	{
		s.restitution		 = j.value<float>("restitution", 0.0f);
		s.friction			 = j.value<float>("friction", 0.2f);
		s.angular_damp		 = j.value<float>("angular_damp", 0.05f);
		s.linear_damp		 = j.value<float>("linear_damp", 0.05f);
		s.mass				 = j.value<float>("mass", 1.0f);
		s.gravity_multiplier = j.value<float>("gravity_multiplier", 1.0f);
	}
#endif

}
