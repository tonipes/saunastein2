// Copyright (c) 2025 Inan Evin

#pragma once
#include "common/size_definitions.hpp"
#include "world/common_entity.hpp"
#include "data/bitmask.hpp"
#include "world/world_constants.hpp"

namespace SFG
{
	enum trait_flags
	{
		trait_flags_is_disabled = 1 << 0,
		trait_flags_is_init		= 1 << 1,
	};

	enum trait_types : uint8
	{
		trait_type_model_instance = 0,
		trait_type_light,
		trait_type_max = 32,
	};

	struct trait_header
	{
		entity_handle  entity;
		bitmask<uint8> flags;
	};
}
