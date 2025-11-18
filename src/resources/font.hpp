// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/bitmask.hpp"
#include "reflection/resource_reflection.hpp"
#include "resources/common_resources.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace vekt
{
	struct font;
}

namespace SFG
{
	struct font_raw;
	class world;

	class font
	{
	public:
		enum flags : uint8
		{
			created = 1 << 0,
		};

		~font();

		// -----------------------------------------------------------------------------
		// resource
		// -----------------------------------------------------------------------------

		void create_from_loader(const font_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline vekt::font* get_vekt_font() const
		{
			return _font;
		}

	private:
		vekt::font*	   _font  = nullptr;
		bitmask<uint8> _flags = 0;

#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
	};

	REGISTER_RESOURCE(font, "stkfont");

}
