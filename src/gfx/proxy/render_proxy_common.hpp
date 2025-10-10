// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"

namespace SFG
{
	enum render_proxy_status : uint8
	{
		rps_inactive = 0,
		rps_active	 = 1,
		rps_obsolete,
	};
}
