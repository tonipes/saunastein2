// Copyright (c) 2025 Inan Evin

#pragma once

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{
	class ostream;
	class istream;

	struct physical_material_settings
	{
		float mass				 = 1.0f;
		float gravity_multiplier = 1.0f;
		float restitution		 = 0.0f;
		float friction			 = 0.2f;
		float angular_damp		 = 0.05f;
		float linear_damp		 = 0.05f;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

#ifdef SFG_TOOLMODE
	void to_json(nlohmann::json& j, const physical_material_settings& s);
	void from_json(const nlohmann::json& j, physical_material_settings& s);
#endif
}
