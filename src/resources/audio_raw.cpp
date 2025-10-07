// Copyright (c) 2025 Inan Evin

#include "audio_raw.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

#ifdef SFG_TOOLMODE
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "project/engine_data.hpp"
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
	bool audio_raw::cook_from_file(const char* path)
	{
		if (!file_system::exists(path))
		{
			SFG_ERR("File don't exist! {0}", path);
			return false;
		}

		try
		{
			std::ifstream f(path);
			json		  json_data = json::parse(f);
			f.close();

			const string& wd		 = engine_data::get().get_working_dir();
			const string  p			 = path;
			name					 = p.substr(wd.size(), p.size() - wd.size());
			source					 = json_data.value<string>("source", "");
			const string full_source = engine_data::get().get_working_dir() + source;
			if (!file_system::exists(full_source.c_str()))
			{
				SFG_ERR("File don't exist! {0}", full_source.c_str());
				return false;
			}

			is_stream = json_data.value<uint8>("stream", 0);

			file_system::read_file_as_vector(full_source.c_str(), audio_data);
			if (audio_data.empty())
			{
				SFG_ERR("Invalid audio data!");
				return false;
			}
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading audio: {0}", e.what());
			return false;
		}

		SFG_INFO("Created audio from file: {0}", path);
		return true;
	}

#endif
}
