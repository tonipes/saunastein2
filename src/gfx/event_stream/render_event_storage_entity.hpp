// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/vector.hpp"
#include "math/matrix4x3.hpp"

namespace SFG
{
	struct render_event_storage_entity
	{
		matrix4x3 abs_model = {};
	};
}
