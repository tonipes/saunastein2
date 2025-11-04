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
		string					name			 = "";
		string					source			 = "";
		vector<compile_variant> compile_variants = {};
		vector<pso_variant>		pso_variants	 = {};

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

		bool compile_specialized(const string& shader_text, const vector<string>& folder_paths, const string& variant_style);
		void destroy();

#ifdef SFG_TOOLMODE
		bool load_from_file(const char* relative_file, const char* base_directory);
		bool load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension);
		void save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const;
		void get_dependencies(vector<string>& out_deps) const;
#endif
	};
}
