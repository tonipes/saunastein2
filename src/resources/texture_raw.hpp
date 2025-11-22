// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/string.hpp"
#include "common/string_id.hpp"
#include "data/static_vector.hpp"
#include "gfx/common/texture_buffer.hpp"
#include "game/game_max_defines.hpp"
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
		bool load_from_file(const char* relative_file, const char* base_path);
		bool load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension);
		void save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const;
		void get_dependencies(vector<string>& out_deps) const;
#endif
	};

}
