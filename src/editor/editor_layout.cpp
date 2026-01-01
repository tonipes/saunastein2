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

#include "editor_layout.hpp"
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "data/string.hpp"

#include <fstream>
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;

namespace SFG
{

	void to_json(nlohmann::json& j, const editor_layout& t)
	{
		j["dummy"] = t.dummy;
	}

	void from_json(const nlohmann::json& j, editor_layout& s)
	{
		s.dummy = j.value<uint32>("dummy", {});
	}

	void editor_layout::init(const char* base_directory)
	{
		const string path = string(base_directory) + "editor_layout.stksettings";

		if (file_system::exists(path.c_str()))
		{
			load(path.c_str());
		}
		else
		{
			// init defaults
			save(path.c_str());
		}
	}

	bool editor_layout::load(const char* path)
	{
		if (!file_system::exists(path))
		{
			SFG_ERR("File don't exist! {0}", path);
			return false;
		}

		try
		{
			std::ifstream f(path);
			editor_layout st = json::parse(f);
			*this			 = st;
			f.close();
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading editor_layout: {0}", e.what());
			return false;
		}

		return true;
	}

	bool editor_layout::save(const char* path)
	{
		json j = *this;

		std::ofstream file(path);
		if (file.is_open())
		{
			file << j.dump(4);
			file.close();
			return true;
		}

		SFG_ERR("failed while writing json! {0}", path);
		return false;
	}
}