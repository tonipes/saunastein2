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

#include "editor_panel_controls.hpp"
#include "math/vector2ui16.hpp"
#include "common/system_info.hpp"
#include "common/string_id.hpp"
#include "memory/memory_tracer.hpp"
#include "platform/process.hpp"

#include "editor/editor_theme.hpp"
#include "editor/editor.hpp"
#include "editor/editor_settings.hpp"

#include "app/app.hpp"

#include "gui/vekt.hpp"
#include "gui/gui_builder.hpp"

namespace SFG
{
	void editor_panel_controls::init(vekt::builder* builder)
	{
		_builder = builder;

		// gui builder
		_gui_builder.style.init_defaults();
		_gui_builder.style.title_font	= editor_theme::get().font_title;
		_gui_builder.style.default_font = editor_theme::get().font_default;

		_gui_builder.init(builder, &editor::get().get_text_allocator());
		_gui_builder.callbacks.on_mouse	 = on_mouse;
		_gui_builder.callbacks.user_data = this;

		_widget = _gui_builder.get_root();

		const string vstr = "stakeforge_engine v." + std::to_string(SFG_MAJOR) + "." + std::to_string(SFG_MINOR) + "." + string(SFG_BUILD);

		// general
		_gui_builder.add_title("general");
		_gui_builder.begin_area(false);
		_gui_builder.add_property_single_label(vstr.c_str());
		_hyperlink = _gui_builder.add_property_single_hyperlink("github");
		_gui_builder.end_area();

		// stats
		_gui_builder.add_title("stats");
		_gui_builder.begin_area();
		_game_res	= _gui_builder.add_property_row_label("game_res:", nullptr).second;
		_window_res = _gui_builder.add_property_row_label("window_res:", nullptr).second;
		_fps		= _gui_builder.add_property_row_label("fps:", nullptr).second;
		_main		= _gui_builder.add_property_row_label("main:", nullptr).second;
		_render		= _gui_builder.add_property_row_label("render:", nullptr).second;
		_ram		= _gui_builder.add_property_row_label("ram:", nullptr).second;
		_vram		= _gui_builder.add_property_row_label("vram:", nullptr).second;
		_vram_txt	= _gui_builder.add_property_row_label("vram_texture:", nullptr).second;
		_vram_res	= _gui_builder.add_property_row_label("vram_buffer:", nullptr).second;
		_draw_calls = _gui_builder.add_property_row_label("draw_calls:", nullptr).second;
		_gui_builder.end_area();

		// level
		_gui_builder.add_title("level");
		_gui_builder.begin_area();
		_loaded_level = _gui_builder.add_property_row_label("level:", nullptr).second;
		// _gui_builder.add_property_row_text_field("lol", "yamate");
		// _gui_builder.add_property_row_text_field("bduudfy", "yeah");
		_gui_builder.add_property_row();
		_button_new_project		 = _gui_builder.set_fill_x(_gui_builder.add_button("new_project").first);
		_button_open_project_dir = _gui_builder.set_fill_x(_gui_builder.add_button("open_project_dir").first);
		_button_package			 = _gui_builder.set_fill_x(_gui_builder.add_button("package_project").first);
		_gui_builder.pop_stack();

		_gui_builder.add_property_row();
		_button_new_level  = _gui_builder.set_fill_x(_gui_builder.add_button("new_level").first);
		_button_save_level = _gui_builder.set_fill_x(_gui_builder.add_button("save_level").first);
		_button_load_level = _gui_builder.set_fill_x(_gui_builder.add_button("load_level").first);
		_gui_builder.pop_stack();
		_gui_builder.end_area();

		_builder->widget_add_child(_builder->get_root(), _widget);
	}

	void editor_panel_controls::uninit()
	{
		_gui_builder.uninit();
	}

	void editor_panel_controls::draw(const vector2ui16& window_size)
	{
		const float wsx = 700.0f;
		const float wsy = 1200.0f;
		_builder->widget_set_pos_abs(_widget, vector2(window_size.x - wsx, 50));
		_builder->widget_set_size_abs(_widget, vector2(700, 1200));

		static float stat_fetch_time = 0.0f;
		static float mem_fetch_time	 = 0.0f;

		static float  stat_main_thread	 = static_cast<float>(frame_info::get_main_thread_time_milli());
		static float  stat_render_thread = static_cast<float>(frame_info::get_render_thread_time_milli());
		static uint32 stat_fps			 = frame_info::get_fps();
		static uint32 stat_dc			 = static_cast<uint32>(frame_info::get_draw_calls());

		static uint32 stat_ram		= 0;
		static uint32 stat_vram		= 0;
		static uint32 stat_vram_txt = 0;
		static uint32 stat_vram_res = 0;

		if (stat_fetch_time > 1500.0f)
		{
			stat_main_thread   = static_cast<float>(frame_info::get_main_thread_time_milli());
			stat_render_thread = static_cast<float>(frame_info::get_render_thread_time_milli());
			stat_fps		   = frame_info::get_fps();
			stat_dc			   = static_cast<uint32>(frame_info::get_draw_calls());
			stat_fetch_time	   = 0.0f;

			size_t ram		= 0;
			size_t vram		= 0;
			size_t vram_txt = 0;
			size_t vram_res = 0;
		}

		if (mem_fetch_time > 6000)
		{
#ifdef SFG_ENABLE_MEMORY_TRACER
			memory_tracer& tracer = memory_tracer::get();
			LOCK_GUARD(tracer.get_category_mtx());

			for (const memory_category& cat : tracer.get_categories())
			{
				if (TO_SID(cat.name) == TO_SID("General"))
				{
					stat_ram = static_cast<uint32>(static_cast<float>(cat.total_size) / (1024 * 1024));
				}
				else if (TO_SID(cat.name) == TO_SID("Gfx"))
				{
					stat_vram = static_cast<uint32>(static_cast<float>(cat.total_size) / (1024 * 1024));
				}
				else if (TO_SID(cat.name) == TO_SID("GfxTxt"))
				{
					stat_vram_txt = static_cast<uint32>(static_cast<float>(cat.total_size) / (1024 * 1024));
				}
				else if (TO_SID(cat.name) == TO_SID("GfxRes"))
				{
					stat_vram_res = static_cast<uint32>(static_cast<float>(cat.total_size) / (1024 * 1024));
				}
			}
#endif
			mem_fetch_time = 0.0f;
		}

		const float ms = static_cast<float>(frame_info::get_main_thread_time_milli());
		mem_fetch_time += ms;
		stat_fetch_time += ms;

		const vector2ui16&	 game_size = editor::get().get_app().get_game_resolution();
		bump_text_allocator& alloc	   = editor::get().get_bump_text_allocator();

		const char* game_res = alloc.allocate_reserve(16);
		alloc.append(game_size.x);
		alloc.append("x");
		alloc.append(game_size.y);

		const char* window_res = alloc.allocate_reserve(16);
		alloc.append(window_size.x);
		alloc.append("x");
		alloc.append(window_size.y);

		const char* fps = alloc.allocate_reserve(6);
		alloc.append(stat_fps);

		const char* main_time = alloc.allocate_reserve(16);
		alloc.append(stat_main_thread);
		alloc.append(" ms");

		const char* render_time = alloc.allocate_reserve(16);
		alloc.append(stat_render_thread);
		alloc.append(" ms");

		const char* ramc = alloc.allocate_reserve(16);
		if (stat_ram)
		{
			alloc.append(stat_ram);
			alloc.append(" mb");
		}
		else
			alloc.append("fetching...");

		const char* vramc = alloc.allocate_reserve(16);
		if (stat_vram)
		{
			alloc.append(stat_ram);
			alloc.append(" mb");
		}
		else
			alloc.append("fetching...");

		const char* vramc_txt = alloc.allocate_reserve(16);
		if (stat_vram_txt)
		{
			alloc.append(stat_ram);
			alloc.append(" mb");
		}
		else
			alloc.append("fetching...");
		const char* vramc_res = alloc.allocate_reserve(16);
		if (stat_ram)
		{
			alloc.append(stat_vram_res);
			alloc.append(" mb");
		}
		else
			alloc.append("fetching...");
		const char* draws = alloc.allocate_reserve(8);
		alloc.append(stat_dc);

		const char* loaded_level = alloc.allocate_reserve(512);
		alloc.append(editor_settings::get().last_world_relative.c_str());

		_builder->widget_get_text(_game_res).text	  = game_res;
		_builder->widget_get_text(_window_res).text	  = window_res;
		_builder->widget_get_text(_fps).text		  = fps;
		_builder->widget_get_text(_main).text		  = main_time;
		_builder->widget_get_text(_render).text		  = render_time;
		_builder->widget_get_text(_ram).text		  = ramc;
		_builder->widget_get_text(_vram).text		  = vramc;
		_builder->widget_get_text(_vram_txt).text	  = vramc_txt;
		_builder->widget_get_text(_vram_res).text	  = vramc_res;
		_builder->widget_get_text(_draw_calls).text	  = draws;
		_builder->widget_get_text(_loaded_level).text = loaded_level;

		_builder->widget_update_text(_game_res);
		_builder->widget_update_text(_window_res);
		_builder->widget_update_text(_fps);
		_builder->widget_update_text(_main);
		_builder->widget_update_text(_render);
		_builder->widget_update_text(_ram);
		_builder->widget_update_text(_vram);
		_builder->widget_update_text(_vram_txt);
		_builder->widget_update_text(_vram_res);
		_builder->widget_update_text(_draw_calls);
		_builder->widget_update_text(_loaded_level);
	}

	vekt::input_event_result editor_panel_controls::on_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		if (ev.type != vekt::input_event_type::pressed)
			return vekt::input_event_result::not_handled;

		editor_panel_controls* ptr = static_cast<editor_panel_controls*>(b->widget_get_user_data(widget).ptr);

		if (widget == ptr->_hyperlink)
		{
			if (b->widget_get_hover_callbacks(widget).is_hovered)
			{
				process::open_url("https://github.com/inanevin/stakeforge");
				return vekt::input_event_result::handled;
			}
		}

		return vekt::input_event_result::not_handled;
	}
}
