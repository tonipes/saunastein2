// Copyright (c) 2025 Inan Evin

#pragma once

#include "common_resources.hpp"
#include "data/bitmask.hpp"
#include "reflection/resource_reflection.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace SFG
{
	struct texture_raw;
	struct world;

	class texture
	{
	public:
		enum flags
		{
			created = 1 << 0,
		};
		~texture();

		void create_from_loader(const texture_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

	private:
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
		uint8		   _texture_format = 0;
		bitmask<uint8> _flags		   = 0;
	};

	REGISTER_RESOURCE(texture, "stktexture");
}