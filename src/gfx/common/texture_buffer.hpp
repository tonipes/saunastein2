// Copyright (c) 2025 Inan Evin
#pragma once

#include "math/vector2ui16.hpp"

namespace SFG
{
	class ostream;
	class istream;

	struct texture_buffer
	{
		uint8*		pixels = nullptr;
		vector2ui16 size   = vector2ui16::zero;
		uint8		bpp	   = 0;

		void serialize(ostream& stream, bool write_addr = false) const;
		void deserialize(istream& stream, bool read_addr = false);
	};

}