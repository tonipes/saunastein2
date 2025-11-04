// Copyright (c) 2025 Inan Evin

#include "texture_sampler_raw.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

#ifdef SFG_TOOLMODE
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "common/string_id.hpp"
#include "serialization/serialization.hpp"
#include <fstream>
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
	void texture_sampler_raw::serialize(ostream& stream) const
	{
		stream << desc;
		stream << name;
	}

	void texture_sampler_raw::deserialize(istream& stream)
	{
		stream >> desc;
		stream >> name;

		SFG_INFO("Created sampler from buffer: {0}", name);
	}

#ifdef SFG_TOOLMODE
	bool texture_sampler_raw::load_from_file(const char* relative_file, const char* base_path)
	{
		const string target_path = base_path + string(relative_file);
		if (!file_system::exists(target_path.c_str()))
		{
			SFG_ERR("File don't exist! {0}", target_path.c_str());
			return false;
		}

		try
		{
			std::ifstream f(target_path);
			json		  json_data = json::parse(f);
			f.close();
			desc = json_data;
			name = relative_file;
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading sampler: {0}", e.what());
			return false;
		}

		SFG_INFO("Created sampler from file: {0}", name);
		return true;
	}

	bool texture_sampler_raw::load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension)
	{
		const string sid_str		 = std::to_string(TO_SID(relative_path));
		const string meta_cache_path = cache_folder_path + sid_str + "_meta" + extension;
		const string data_cache_path = cache_folder_path + sid_str + "_data" + extension;

		if (!file_system::exists(meta_cache_path.c_str()))
			return false;

		if (!file_system::exists(data_cache_path.c_str()))
			return false;

		istream stream = serialization::load_from_file(meta_cache_path.c_str());

		string file_path				= "";
		uint64 saved_file_last_modified = 0;
		stream >> file_path;
		stream >> saved_file_last_modified;

		stream.destroy();

		const uint64 file_last_modified = file_system::get_last_modified_ticks(file_path);

		if (file_last_modified != saved_file_last_modified)
			return false;

		stream = serialization::load_from_file(data_cache_path.c_str());
		deserialize(stream);
		stream.destroy();
		return true;
	}

	void texture_sampler_raw::save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const
	{
		const string sid_str			= std::to_string(TO_SID(name));
		const string file_path			= resource_directory_path + name;
		const uint64 file_last_modified = file_system::get_last_modified_ticks(file_path);

		const string meta_cache_path = cache_folder_path + sid_str + "_meta" + extension;
		const string data_cache_path = cache_folder_path + sid_str + "_data" + extension;

		ostream out_stream;
		out_stream << file_path;
		out_stream << file_last_modified;
		serialization::save_to_file(meta_cache_path.c_str(), out_stream);

		out_stream.shrink(0);
		serialize(out_stream);
		serialization::save_to_file(data_cache_path.c_str(), out_stream);

		out_stream.destroy();
	}

#endif
}
