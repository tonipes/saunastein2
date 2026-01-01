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

#pragma once

#include "math/vector4.hpp"
#include "data/string.hpp"
#include <vendor/nhlohmann/json_fwd.hpp>

namespace SFG
{
	struct editor_theme
	{
		float	default_indent	= 8.0f;
		vector4 col_bg_frame	= vector4(3, 3, 3, 255) / 255.0f;
		vector4 col_bg_child	= vector4(8, 8, 8, 255) / 255.0f;
		vector4 col_bg_window	= vector4(16, 16, 16, 255) / 255.0f;
		vector4 col_accent_prim = vector4(231, 63, 71, 255) / 255.0f;
		vector4 col_accent_sec	= vector4(231, 63, 71, 255) / 255.0f;
		vector4 color_axis_x	= vector4(204.0f, 51.0f, 51.0f, 255.0f) / 255.0f;
		vector4 color_axis_y	= vector4(51.0f, 204.0f, 51.0f, 255.0f) / 255.0f;
		vector4 color_axis_z	= vector4(51.0f, 51.0f, 204.0f, 255.0f) / 255.0f;

		void init(const char* base_directory);
		bool load(const char* path);
		bool save(const char* path);

		inline bool save_last()
		{
			save(_last_path.c_str());
		}

		string _last_path = "";
	};

	void to_json(nlohmann::json& j, const editor_theme& t);
	void from_json(const nlohmann::json& j, editor_theme& s);
}
