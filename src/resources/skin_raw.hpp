// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "common_skin.hpp"
#include "data/vector.hpp"
#include "common/string_id.hpp"
#include "data/string.hpp"

namespace SFG
{
	class ostream;
	class istream;

	struct skin_raw
	{
		string			   name = "";
		string_id		   sid	= 0;
		vector<skin_joint> joints;
		int16			   root_joint = -1;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

		void save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const
		{
		}
		bool load_from_file(const char* relative_file, const char* base_path)
		{
			return false;
		};
		bool load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension)
		{
			return false;
		}
		void get_dependencies(vector<string>& out_deps) const {};
	};
}
