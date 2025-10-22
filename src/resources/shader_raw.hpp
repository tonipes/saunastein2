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
		bool load_from_file(const char* file);
		bool load_from_file(const char* file, const char* base_directory_for_source);
		void get_dependencies(vector<string>& out_deps) const;
#endif
	};
}
