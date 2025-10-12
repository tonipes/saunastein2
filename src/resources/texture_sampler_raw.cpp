// Copyright (c) 2025 Inan Evin

#include "texture_sampler_raw.hpp"
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
	bool texture_sampler_raw::cook_from_file(const char* path)
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
			desc = json_data;

			const string& wd = engine_data::get().get_working_dir();
			const string  p	 = path;
			name			 = p.substr(wd.size(), p.size() - wd.size());
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading sampler: {0}", e.what());
			return false;
		}

		SFG_INFO("Created sampler from file: {0}", name);
		return true;
	}

#endif
}
