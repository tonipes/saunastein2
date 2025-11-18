// Copyright (c) 2025 Inan Evin

#pragma once

#include "resources/common_resources.hpp"
#include "reflection/resource_reflection.hpp"
#include "data/bitmask.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace SFG
{
	struct shader_raw;
	class world;

	class shader
	{
	public:
		enum flags
		{
			created = 1 << 0,
		};

		~shader();

		// -----------------------------------------------------------------------------
		// resource
		// -----------------------------------------------------------------------------

		void create_from_loader(const shader_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

	private:
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
		bitmask<uint8> _flags = 0;
	};

	REGISTER_RESOURCE(shader, "stkshader");

}
