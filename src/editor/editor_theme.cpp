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
#include "math/color.hpp"
#include <vendor/nhlohmann/json.hpp>
#include <fstream>
using json = nlohmann::json;

namespace SFG
{
	float editor_theme::DPI_SCALE = 1.0f;

	void to_json(nlohmann::json& j, const editor_theme& t)
	{
		// j["default_indent"]	 = t.default_indent;
	}

	void from_json(const nlohmann::json& j, editor_theme& s)
	{
		//s.default_indent  = j.value<float>("default_indent", 0.0f);
	
	}

	void editor_theme::init_defaults()
	{
		col_accent			  = color::from255(151.0f, 0.0f, 119.0f, 255.0f).srgb_to_linear().to_vector();
		col_accent_second	  = color::from255(7, 131, 214, 255.0f).srgb_to_linear().to_vector();
		col_accent_second_dim = color::from255(7, 131, 214, 150.0f).srgb_to_linear().to_vector();
		col_title_line_start  = color::from255(91.0f, 0.0f, 72.0f, 0.0f).srgb_to_linear().to_vector();
		col_title_line_end	  = color::from255(151.0f, 0.0f, 119.0f, 255.0f).srgb_to_linear().to_vector();
		col_hyperlink		  = color::from255(7, 131, 214, 255.0f).srgb_to_linear().to_vector();

		col_highlight				= col_accent_second;
		col_highlight_transparent	= col_accent_second;
		col_highlight_transparent.w = 0.5f;

		col_title	 = color::from255(180, 180, 180, 255).srgb_to_linear().to_vector();
		col_text	 = color::from255(180, 180, 180, 255).srgb_to_linear().to_vector();
		col_text_dim = color::from255(130, 130, 130, 255).srgb_to_linear().to_vector();
		col_frame_bg = color::from255(4, 4, 4, 255).srgb_to_linear().to_vector();
		col_area_bg	 = color::from255(15, 15, 15, 255).srgb_to_linear().to_vector();
		col_root	 = color::from255(28, 28, 28, 255).srgb_to_linear().to_vector();

		col_scroll_bar			 = col_accent;
		col_scroll_bar_bg		 = col_frame_bg;
		col_button				 = col_root;
		col_button_hover		 = col_area_bg;
		col_button_press		 = col_frame_bg;
		col_frame_outline		 = color::from255(60, 60, 60, 255).srgb_to_linear().to_vector();
		col_context_menu_outline = color::from255(60, 60, 60, 255).srgb_to_linear().to_vector();

		root_rounding = 6.0f;

		outer_margin	  = DPI_SCALE * 8;
		item_spacing	  = DPI_SCALE * 3;
		root_spacing	  = DPI_SCALE * 6;
		row_spacing		  = DPI_SCALE * 6;
		row_height		  = DPI_SCALE * 20;
		title_line_width  = 0.8f;
		title_line_height = DPI_SCALE * 2;

		item_height			= DPI_SCALE * 16;
		table_cell_height	= DPI_SCALE * 10;
		seperator_thickness = DPI_SCALE * 1;
		property_cell_div	= 0.3f;

		area_rounding	 = 8.0f;
		scroll_thickness = DPI_SCALE * 4;
		scroll_rounding	 = 8.0f;

		inner_margin				   = DPI_SCALE * 4;
		frame_thickness				   = DPI_SCALE * 1;
		frame_rounding				   = 2.0f;
		context_menu_outline_thickness = DPI_SCALE * 1.2f;
	}
	void editor_theme::init(const char* base_directory)
	{
		const string last_path = string(base_directory) + "editor_theme.stksettings";

		if (file_system::exists(last_path.c_str()))
		{
			load(last_path.c_str());
		}
		else
		{
			init_defaults();
			save(last_path.c_str());
		}
		_last_path = last_path;
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