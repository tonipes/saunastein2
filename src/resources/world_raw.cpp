/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "world_raw.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

#ifdef SFG_TOOLMODE
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "editor/editor_settings.hpp"
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
	bool world_raw::load_from_file(const char* relative_file, const char* base_path)
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

			resources = json_data.value<vector<string>>("resources", {});

			for (const string& res : resources)
			{
				const string source = editor_settings::get().working_dir + res;
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

		SFG_INFO("Created world from file: {0}", target_path);
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
