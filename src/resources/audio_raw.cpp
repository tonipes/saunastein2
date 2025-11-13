// Copyright (c) 2025 Inan Evin

#include "audio_raw.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

#ifdef SFG_TOOLMODE
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "serialization/serialization.hpp"
#include "common/string_id.hpp"
#include <fstream>
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
	void audio_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << source;
		stream << is_stream;
		stream << audio_data;
	}

	void audio_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> source;
		stream >> is_stream;
		stream >> audio_data;
	}
#ifdef SFG_TOOLMODE

	bool audio_raw::load_from_file(const char* relative_file, const char* base_path)
	{
		const string target_path = base_path + string(relative_file);
		if (!file_system::exists(target_path.c_str()))
		{
			SFG_ERR("File don't exist! {0}", target_path.c_str());
			return false;
		}

		try
		{
			std::ifstream f(target_path.c_str());
			json		  json_data = json::parse(f);
			f.close();

			name   = relative_file;
			source = json_data.value<string>("source", "");

			const string full_source = base_path + source;
			if (!file_system::exists(full_source.c_str()))
			{
				SFG_ERR("File don't exist! {0}", full_source.c_str());
				return false;
			}

			is_stream = json_data.value<uint8>("stream", 0);

			char*  file_data	  = nullptr;
			size_t file_data_size = 0;
			file_system::read_file(full_source.c_str(), file_data, file_data_size);

			if (file_data_size == 0)
			{
				SFG_ERR("Invalid audio data!");
				return false;
			}

			audio_data.resize(file_data_size);
			SFG_MEMCPY(audio_data.data(), file_data, file_data_size);
			delete[] file_data;
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading audio: {0}", e.what());
			return false;
		}

		SFG_INFO("Created audio from file: {0}", target_path.c_str());
		return true;
	}

	/*
	bool resource_manager::load_from_cache(string_id type, void*& loader, const char* releative_path)
	{
		const string cache_path = engine_data::get().get_cache_dir() + std::to_string(TO_SID(releative_path)) + ".stkcache";

		if (!file_system::exists(cache_path.c_str()))
			return false;

		istream stream = serialization::load_from_file(cache_path.c_str());

		const string base_path = engine_data::get().get_working_dir() + releative_path;

		if (!file_system::exists(base_path.c_str()))
		{
			stream.destroy();
			return false;
		}

		const string_id file_last_modified = file_system::get_last_modified_ticks(base_path);

		string_id stream_base_last_modified = 0;
		stream >> stream_base_last_modified;

		if (stream_base_last_modified != file_last_modified)
		{
			stream.destroy();
			return false;
		}

		uint32 dep_size = 0;
		stream >> dep_size;

		for (uint32 i = 0; i < dep_size; i++)
		{
			string_id dep_modified = 0;
			string	  dep_relative = "";
			stream >> dep_modified;
			stream >> dep_relative;

			const string p = engine_data::get().get_working_dir() + dep_relative;
			if (!file_system::exists(p.c_str()))
				continue;

			const string_id modified = file_system::get_last_modified_ticks(p);
			if (modified != dep_modified)
			{
				stream.destroy();
				return false;
			}
		}

		loader = load_from_stream(type, stream);
		stream.destroy();
		return true;
	}

	void resource_manager::save_to_cache(string_id type, const void* loader, const char* relative_path)
	{
		vector<string> dependencies;
		get_dependencies(type, loader, dependencies);

		ostream out_stream;

		const string	base_path		   = engine_data::get().get_working_dir() + relative_path;
		const string_id file_last_modified = file_system::get_last_modified_ticks(base_path);
		out_stream << file_last_modified;
		out_stream << static_cast<uint32>(dependencies.size());

		for (const string& dep : dependencies)
		{
			const string	p		 = engine_data::get().get_working_dir() + dep;
			const string_id modified = file_system::get_last_modified_ticks(p);
			out_stream << modified;
			out_stream << dep;
		}

		save_to_stream(type, loader, out_stream);
		const string cache_path = engine_data::get().get_cache_dir() + std::to_string(TO_SID(relative_path)) + ".stkcache";
		serialization::save_to_file(cache_path.c_str(), out_stream);
		out_stream.destroy();
	}
	*/

	bool audio_raw::load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension)
	{
		const string sid_str		 = std::to_string(TO_SID(relative_path));
		const string meta_cache_path = cache_folder_path + sid_str + "_meta" + extension;
		const string data_cache_path = cache_folder_path + sid_str + "_data" + extension;

		if (!file_system::exists(meta_cache_path.c_str()))
			return false;

		if (!file_system::exists(data_cache_path.c_str()))
			return false;

		istream stream = serialization::load_from_file(meta_cache_path.c_str());

		string file_path				  = "";
		string source_path				  = "";
		uint64 saved_file_last_modified	  = 0;
		uint64 saved_source_last_modified = 0;
		stream >> file_path;
		stream >> source_path;
		stream >> saved_file_last_modified;
		stream >> saved_source_last_modified;

		stream.destroy();

		const uint64 file_last_modified = file_system::get_last_modified_ticks(file_path);
		const uint64 src_last_modified	= file_system::get_last_modified_ticks(source_path);

		if (file_last_modified != saved_file_last_modified || src_last_modified != saved_source_last_modified)
			return false;

		stream = serialization::load_from_file(data_cache_path.c_str());
		deserialize(stream);
		stream.destroy();
		return true;
	}

	void audio_raw::save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const
	{
		const string sid_str			= std::to_string(TO_SID(name));
		const string file_path			= resource_directory_path + name;
		const string source_path		= resource_directory_path + source;
		const uint64 file_last_modified = file_system::get_last_modified_ticks(file_path);
		const uint64 src_last_modified	= file_system::get_last_modified_ticks(source_path);

		const string meta_cache_path = cache_folder_path + sid_str + "_meta" + extension;
		const string data_cache_path = cache_folder_path + sid_str + "_data" + extension;

		ostream out_stream;
		out_stream << file_path;
		out_stream << source_path;
		out_stream << file_last_modified;
		out_stream << src_last_modified;
		serialization::save_to_file(meta_cache_path.c_str(), out_stream);

		out_stream.shrink(0);
		serialize(out_stream);
		serialization::save_to_file(data_cache_path.c_str(), out_stream);

		out_stream.destroy();
	}

	void audio_raw::get_dependencies(vector<string>& out_deps) const
	{
		out_deps.push_back(source);
	}

#endif
}
