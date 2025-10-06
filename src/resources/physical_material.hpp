// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "common/type_id.hpp"
#include "resources/common_resources.hpp"
#include "memory/chunk_handle.hpp"
namespace SFG
{
	struct physical_material_raw;

	struct physical_material_reflection
	{
		physical_material_reflection();
	};

	extern physical_material_reflection g_physical_material_reflection;

	class world;
	class chunk_allocator32;

	class physical_material
	{
	public:
		void create_from_raw(const physical_material_raw& raw, chunk_allocator32& alloc);
		void destroy(chunk_allocator32& alloc);

	private:
		chunk_handle32 _name = {};
	};

	REGISTER_TYPE(physical_material, resource_type::resource_type_physical_material);

}
