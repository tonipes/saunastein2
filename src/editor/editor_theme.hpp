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

namespace vekt
{
	struct font;
}

namespace SFG
{
	struct editor_theme
	{
		vekt::font* font_title	 = nullptr;
		vekt::font* font_default = nullptr;
		vekt::font* font_icons	 = nullptr;

		static float DPI_SCALE;

		float root_rounding;
		float root_spacing;
		float outer_margin;
		float item_spacing;
		float row_spacing;
		float title_line_width;
		float title_line_height;
		float item_height;
		float row_height;
		float table_cell_height;
		float property_cell_div;
		float seperator_thickness;
		float area_rounding;
		float scroll_thickness;
		float scroll_rounding;
		float inner_margin;
		float frame_thickness;
		float frame_rounding;
		float context_menu_outline_thickness;

		vector4 col_title_line_start;
		vector4 col_title_line_end;
		vector4 col_hyperlink;
		vector4 col_accent;
		vector4 col_accent_second;
		vector4 col_accent_second_dim;
		vector4 col_highlight;
		vector4 col_highlight_transparent;
		vector4 col_scroll_bar;
		vector4 col_scroll_bar_bg;
		vector4 col_title;
		vector4 col_text;
		vector4 col_text_dim;
		vector4 col_frame_bg;
		vector4 col_area_bg;
		vector4 col_root;
		vector4 col_button;
		vector4 col_button_hover;
		vector4 col_button_press;
		vector4 col_frame_outline;
		vector4 col_context_menu_outline;

		vector4 color_axis_x = vector4(204.0f, 51.0f, 51.0f, 255.0f) / 255.0f;
		vector4 color_axis_y = vector4(51.0f, 204.0f, 51.0f, 255.0f) / 255.0f;
		vector4 color_axis_z = vector4(51.0f, 51.0f, 204.0f, 255.0f) / 255.0f;

		static editor_theme& get()
		{
			static editor_theme et;
			return et;
		}

		void init(const char* base_directory);
		void init_defaults();
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
