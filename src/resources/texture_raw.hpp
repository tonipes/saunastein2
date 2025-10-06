// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/string.hpp"
#include "common/string_id.hpp"
#include "data/static_vector.hpp"
#include "gfx/common/texture_buffer.hpp"
#include "gfx/common/gfx_constants.hpp"

namespace SFG
{
	class ostream;
	class istream;
	struct vector2ui16;

	struct texture_raw
	{
		string											name		   = "";
		uint8											texture_format = 0;
		static_vector<texture_buffer, MAX_TEXTURE_MIPS> buffers;
		string_id										sid = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

		void cook_from_data(uint8* base, const vector2ui16& size, uint8 format, bool generate_mips);

#ifdef SFG_TOOLMODE
		bool cook_from_file(const char* file);
#endif
	};

}
