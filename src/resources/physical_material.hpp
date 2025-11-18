// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/bitmask.hpp"
#include "resources/common_resources.hpp"
#include "reflection/resource_reflection.hpp"
#include "physics/physics_material_settings.hpp"

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
		enum flags
		{
			created = 1 << 0,
		};

		~physical_material();

		// -----------------------------------------------------------------------------
		// resource
		// -----------------------------------------------------------------------------

		void create_from_loader(const physical_material_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline physical_material_settings& get_settings()
		{
			return _settings;
		}

	private:
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif

		physical_material_settings _settings = {};
		bitmask<uint8>			   _flags	 = 0;
	};

	REGISTER_RESOURCE(physical_material, "stkphymat");

}
