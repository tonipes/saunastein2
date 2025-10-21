// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "gfx/common/descriptions.hpp"
#include "data/string.hpp"

namespace SFG
{
	class ostream;
	class istream;

	struct texture_sampler_raw
	{
		sampler_desc desc = {};
		string		 name = "";

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

#ifdef SFG_TOOLMODE
		bool load_from_file(const char* file);
		void get_dependencies(vector<string>& out_deps) const {};
#endif
	};
}
