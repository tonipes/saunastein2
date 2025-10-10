// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/string_id.hpp"
#include "data/string.hpp"

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{
	struct resource_ident
	{
		string	  relative_name = "";
		string_id sid			= 0;
	};

#ifdef SFG_TOOLMODE
	void to_json(nlohmann::json& j, const resource_ident& s);
	void from_json(const nlohmann::json& j, resource_ident& s);
#endif
}
