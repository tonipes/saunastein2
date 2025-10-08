// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "memory/chunk_handle.hpp"
#include "resources/common_resources.hpp"
#include "reflection/resource_reflection.hpp"

namespace SFG
{
	class chunk_allocator32;
	struct skin_raw;

	struct skin_reflection
	{
		skin_reflection();
	};

	class skin
	{
	public:
		void create_from_raw(const skin_raw& raw, chunk_allocator32& alloc);
		void destroy(chunk_allocator32& alloc);

	private:
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
		chunk_handle32 _joints;
		uint16		   _joints_count = 0;
		int16		   _root		 = -1;
	};

	REGISTER_RESOURCE(skin, resource_type_skin, skin_reflection);

}