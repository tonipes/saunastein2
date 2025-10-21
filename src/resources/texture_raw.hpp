// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/string.hpp"
#include "common/string_id.hpp"
#include "data/static_vector.hpp"
#include "gfx/common/texture_buffer.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "data/vector.hpp"

namespace SFG
{
	class ostream;
	class istream;
	struct vector2ui16;

	struct texture_raw
	{
		string											name		   = "";
		string											source		   = "";
		uint8											texture_format = 0;
		static_vector<texture_buffer, MAX_TEXTURE_MIPS> buffers;
		string_id										sid = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

		void load_from_data(uint8* base, const vector2ui16& size, uint8 format, bool generate_mips);

#ifdef SFG_TOOLMODE
		bool load_from_file(const char* file);
		void get_dependencies(vector<string>& out_deps) const;
#endif
	};

}
