// Copyright (c) 2025 Inan Evin

#include "resource_ident.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
#ifdef SFG_TOOLMODE

	void to_json(nlohmann::json& j, const resource_ident& s)
	{
		j["relative_name"] = s.relative_name;
		j["sid"]		   = TO_SID(s.relative_name);
	}
	void from_json(const nlohmann::json& j, resource_ident& s)
	{
		s.relative_name = j.value<string>("relative_name", "");
		s.sid			= TO_SID(s.relative_name);
	}
#endif
}
