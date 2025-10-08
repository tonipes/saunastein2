// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "resources/common_resources.hpp"
#include "reflection/resource_reflection.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace SFG
{
	struct physical_material_raw;

	struct physical_material_reflection
	{
		physical_material_reflection();
	};

	class world;
	class chunk_allocator32;

	class physical_material
	{
	public:
		void create_from_raw(const physical_material_raw& raw, chunk_allocator32& alloc);
		void destroy(chunk_allocator32& alloc);

	private:
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
	};

	REGISTER_RESOURCE(physical_material, resource_type::resource_type_physical_material, physical_material_reflection);

}
