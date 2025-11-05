// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/static_vector.hpp"
#include "data/bitmask.hpp"
#include "common/size_definitions.hpp"
#include "gfx/common/gfx_constants.hpp"

namespace SFG
{
	struct shader_raw;

	class shader_direct
	{

	private:
		struct hw
		{
			gfx_id id	 = 0;
			bitmask<uint32> flags;
		};

	public:
		~shader_direct();

		void create_from_loader(shader_raw& raw, gfx_id layout);
		void destroy();

		uint16 get_hw(uint32 variant_flags) const;

		inline uint16 get_hw() const
		{
			return _hws[0].id;
		}

	private:
		static_vector<hw, 8> _hws;
	};

}
