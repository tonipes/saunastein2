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

	struct font_raw
	{
		string		 name		  = "";
		string		 source		  = "";
		uint16		 point_size	  = 0;
		vector<char> font_data	  = {};
		uint8		 font_type	  = 0;
		int16		 sdf_padding  = 0;
		int16		 sdf_edge	  = 0;
		float		 sdf_distance = 0.0f;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

#ifdef SFG_TOOLMODE
		bool load_from_file(const char* relative_file, const char* base_path);
		bool load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension);
		void save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const;
		void get_dependencies(vector<string>& out_deps) const;
#endif
	};
}
