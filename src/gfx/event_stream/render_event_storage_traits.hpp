// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/vector.hpp"

namespace SFG
{
	struct render_event_storage_model_instance
	{
		uint16		   model = 0;
		uint16		   mesh	 = 0;
		vector<uint16> materials;
		uint8		   single_mesh = 0;
	};
}
