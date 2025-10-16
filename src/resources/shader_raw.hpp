// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/string.hpp"
#include "data/vector.hpp"
#include "data/bitmask.hpp"
#include "gfx/common/shader_description.hpp"

namespace SFG
{
	class ostream;
	class istream;

	struct shader_raw
	{
		string		   name		  = "";
		string		   source	  = "";
		shader_desc	   desc		  = {};
		vector<string> defines	  = {};
		uint8		   is_skinned = 0;
		uint8		   is_discard = 0;

		void destroy();
		void serialize(ostream& stream) const;
		void deserialize(istream& stream, bool use_embedded_layout, gfx_id layout);

#ifdef SFG_TOOLMODE
		bool cook_from_file(const char* file, bool use_embedded_layout, gfx_id layout);
#endif
	};
}
