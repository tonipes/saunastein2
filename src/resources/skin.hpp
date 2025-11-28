// Copyright (c) 2025 Inan Evin
#pragma once

#include "memory/chunk_handle.hpp"
#include "resources/common_resources.hpp"
#include "reflection/resource_reflection.hpp"

namespace SFG
{
	struct skin_raw;
	class world;

	class skin
	{
	public:
		// -----------------------------------------------------------------------------
		// resource
		// -----------------------------------------------------------------------------

		void create_from_loader(const skin_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline chunk_handle32 get_joints() const
		{
			return _joints;
		}

		inline uint16 get_joints_count() const
		{
			return _joints_count;
		}

	private:
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
		chunk_handle32 _joints;
		uint16		   _joints_count = 0;
		int16		   _root		 = -1;
	};

	REGISTER_RESOURCE(skin, "");

}