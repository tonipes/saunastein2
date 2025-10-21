// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "gfx/common/descriptions.hpp"
#include "data/vector.hpp"

namespace SFG
{
	class ostream;
	class istream;
	class world;

	struct world_raw
	{
		vector<string> resources;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

#ifdef SFG_TOOLMODE
		bool load_from_file(const char* file);
		void save_to_file(const char* file, world& w);
		void fetch_from_world(world& w);
#endif
	};
}
