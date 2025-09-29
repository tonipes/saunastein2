// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "common/type_id.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{
	struct physical_material_raw;

	struct physical_material_reflection
	{
		physical_material_reflection();
	};

	extern physical_material_reflection g_physical_material_reflection;

	class world;

	class physical_material
	{
	public:
		void create_from_raw(const physical_material_raw& raw);
		void destroy();

	private:
	};

	REGISTER_TYPE(physical_material, resource_type::resource_type_physical_material);

}
