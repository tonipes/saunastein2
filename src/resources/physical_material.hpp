// Copyright (c) 2025 Inan Evin

#pragma once

#include "resources/common_resources.hpp"
#include "reflection/resource_reflection.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace SFG
{
	struct physical_material_raw;
	class world;

	class physical_material
	{
	public:
		void create_from_loader(const physical_material_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

	private:
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
	};

	REGISTER_RESOURCE(physical_material, "stkphymat");

}
