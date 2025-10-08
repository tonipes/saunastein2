// Copyright (c) 2025 Inan Evin

#pragma once
#include "common/size_definitions.hpp"
#include "memory/pool_handle.hpp"

namespace SFG
{
#define MAX_RENDERABLE_NODES	 1024
#define MAX_RENDERABLE_MATERIALS 256
#define MAX_ENTITIES			 512
#define MAX_TRAIT_AUX_MEMORY	 1024

	typedef uint32		  entity_id;
	typedef pool_handle32 entity_handle;
	typedef pool_handle32 trait_handle;

#define NULL_ENTITY_ID std::numeric_limits<entity_id>::max()

}
