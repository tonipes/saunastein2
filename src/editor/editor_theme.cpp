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

#include "editor_theme.hpp"
#include "io/file_system.hpp"
#include "io/log.hpp"
#include <vendor/nhlohmann/json.hpp>
#include <fstream>
using json = nlohmann::json;

namespace SFG
{
	void to_json(nlohmann::json& j, const editor_theme& t)
	{
		j["default_indent"]	 = t.default_indent;
		j["col_bg_frame"]	 = t.col_bg_frame;
		j["col_bg_child"]	 = t.col_bg_child;
		j["col_bg_window"]	 = t.col_bg_window;
		j["col_accent_prim"] = t.col_accent_prim;
		j["col_accent_sec"]	 = t.col_accent_sec;
	}

	void from_json(const nlohmann::json& j, editor_theme& s)
	{
		s.default_indent  = j.value<float>("default_indent", 0.0f);
		s.col_bg_frame	  = j.value<vector4>("col_bg_frame", vector4::zero);
		s.col_bg_child	  = j.value<vector4>("col_bg_child", vector4::zero);
		s.col_bg_window	  = j.value<vector4>("col_bg_window", vector4::zero);
		s.col_accent_prim = j.value<vector4>("col_accent_prim", vector4::zero);
		s.col_accent_sec  = j.value<vector4>("col_accent_sec", vector4::zero);
	}

	void editor_theme::init(const char* base_directory)
	{
		 _last_path = string(base_directory) + "editor_theme.stksettings";

		if (file_system::exists(_last_path.c_str()))
		{
			load(_last_path.c_str());
		}
		else
		{
			// init defaults
			save(_last_path.c_str());
		}
	}

	bool editor_theme::load(const char* path)
	{
		if (!file_system::exists(path))
		{
			SFG_ERR("File don't exist! {0}", path);
			return false;
		}

		try
		{
			std::ifstream f(path);
			editor_theme  st = json::parse(f);
			*this			 = st;
			f.close();
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading editor_theme: {0}", e.what());
			return false;
		}

		return true;
	}

	bool editor_theme::save(const char* path)
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