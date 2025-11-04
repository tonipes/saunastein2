// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "gfx/common/descriptions.hpp"
#include "data/string.hpp"
#include "data/vector.hpp"

namespace SFG
{
	class ostream;
	class istream;

	struct physical_material_raw
	{
		string name			= "";
		float  restitution	= 0.0f;
		float  friction		= 0.0f;
		float  angular_damp = 0.0f;
		float  linear_damp	= 0.0f;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

#ifdef SFG_TOOLMODE
		bool load_from_file(const char* relative_file, const char* base_path);
		bool load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension);
		void save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const;
		void get_dependencies(vector<string>& out_deps) const {};
#endif
	};
}
