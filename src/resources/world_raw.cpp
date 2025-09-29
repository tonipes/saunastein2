// Copyright (c) 2025 Inan Evin

#include "world_raw.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

#ifdef SFG_TOOLMODE
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "project/engine_data.hpp"
#include "gui/vekt.hpp"
#include "world/world.hpp"
#include <fstream>
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
	void world_raw::serialize(ostream& stream) const
	{
		stream << resources;
	}

	void world_raw::deserialize(istream& stream)
	{
		stream >> resources;
	}

#ifdef SFG_TOOLMODE
	bool world_raw::cook_from_file(const char* path)
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

			const vector<string> resources = json_data.value<vector<string>>("resources", {});

			for (const string& res : resources)
			{
				const string source = engine_data::get().get_working_dir() + res;
				if (!file_system::exists(source.c_str()))
				{
					SFG_ERR("File don't exist! {0}", source.c_str());
					return false;
				}
			}
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading world: {0}", e.what());
			return false;
		}

		SFG_INFO("Created world from file: {0}", path);
		return true;
	}

	void world_raw::save_to_file(const char* path, world& w)
	{
		json j		   = {};
		j["resources"] = resources;

		std::ofstream file(path);
		if (file.is_open())
		{
			file << j.dump(4);
			file.close();
		}
	}

	void world_raw::fetch_from_world(world& w)
	{
	}

#endif
}
