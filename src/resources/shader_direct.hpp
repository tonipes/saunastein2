// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "gfx/common/gfx_constants.hpp"
#include <limits>

namespace SFG
{
	struct shader_raw;

	class shader_direct
	{
	public:
		~shader_direct();

		void create_from_raw(shader_raw& raw);
		void destroy();

		inline uint16 get_hw() const
		{
			return _hw;
		}

	private:
		gfx_id _hw = NULL_GFX_ID;
	};

}
