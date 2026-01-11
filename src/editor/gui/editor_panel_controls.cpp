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

namespace SFG
{
	void editor_panel_controls::init(vekt::builder* builder)
	{
		_builder = builder;

		// gui builder
		// gui_builder now uses editor_theme directly for styles and fonts.

		_gui_builder.init(builder);
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
		_game_res	= _gui_builder.add_property_row_label("game_res:", "fetching...", 12).second;
		_window_res = _gui_builder.add_property_row_label("window_res:", "fetching...", 12).second;
		_fps		= _gui_builder.add_property_row_label("fps:", "fetching...", 12).second;
		_main		= _gui_builder.add_property_row_label("main:", "fetching...", 12).second;
		_render		= _gui_builder.add_property_row_label("render:", "fetching...", 12).second;
		_ram		= _gui_builder.add_property_row_label("ram:", "fetching...", 12).second;
		_vram		= _gui_builder.add_property_row_label("vram:", "fetching...", 12).second;
		_vram_txt	= _gui_builder.add_property_row_label("vram_texture:", "fetching...", 12).second;
		_vram_res	= _gui_builder.add_property_row_label("vram_buffer:", "fetching...", 12).second;
		_draw_calls = _gui_builder.add_property_row_label("draw_calls:", "fetching...", 12).second;
		_gui_builder.end_area();

		// level
		_gui_builder.add_title("level");
		_gui_builder.begin_area();
		_loaded_level = _gui_builder.add_property_row_label("world:", "none", 256).second;
		// _gui_builder.add_property_row_text_field("lol", "yamate");
		// _gui_builder.add_property_row_text_field("bduudfy", "yeah");
		_gui_builder.add_property_row();
		_button_new_project		 = _gui_builder.set_fill_x(_gui_builder.add_button("new_project").first);
		_button_open_project_dir = _gui_builder.set_fill_x(_gui_builder.add_button("open_project_dir").first);
		_button_package			 = _gui_builder.set_fill_x(_gui_builder.add_button("package_project").first);
		_gui_builder.pop_stack();

		_gui_builder.add_property_row();
		_button_new_level  = _gui_builder.set_fill_x(_gui_builder.add_button("new_world").first);
		_button_save_level = _gui_builder.set_fill_x(_gui_builder.add_button("save_world").first);
		_button_load_level = _gui_builder.set_fill_x(_gui_builder.add_button("load_world").first);
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
		static uint32 stat_ram			 = 0;
		static uint32 stat_vram			 = 0;
		static uint32 stat_vram_txt		 = 0;
		static uint32 stat_vram_res		 = 0;

		if (stat_fetch_time > 1500.0f)
		{
			stat_main_thread   = static_cast<float>(frame_info::get_main_thread_time_milli());
			stat_render_thread = static_cast<float>(frame_info::get_render_thread_time_milli());
			stat_fps		   = frame_info::get_fps();
			stat_dc			   = static_cast<uint32>(frame_info::get_draw_calls());
			stat_fetch_time	   = 0.0f;

			const vector2ui16& game_size = editor::get().get_app().get_game_resolution();

			_builder->widget_append_text_start(_game_res);
			_builder->widget_append_text(_game_res, game_size.x, 0);
			_builder->widget_append_text(_game_res, "x");
			_builder->widget_append_text(_game_res, game_size.y, 0);

			_builder->widget_append_text_start(_window_res);
			_builder->widget_append_text(_window_res, window_size.x, 0);
			_builder->widget_append_text(_window_res, "x");
			_builder->widget_append_text(_window_res, window_size.y, 0);

			_builder->widget_append_text_start(_fps);
			_builder->widget_append_text(_fps, stat_fps);

			_builder->widget_append_text_start(_main);
			_builder->widget_append_text(_main, stat_main_thread);
			_builder->widget_append_text(_main, " ms");

			_builder->widget_append_text_start(_render);
			_builder->widget_append_text(_render, stat_render_thread);
			_builder->widget_append_text(_render, " ms");

			_builder->widget_append_text_start(_loaded_level);
			_builder->widget_append_text(_loaded_level, editor::get().get_loaded_level().c_str());

			_builder->widget_append_text_start(_draw_calls);
			_builder->widget_append_text(_draw_calls, stat_dc);
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

			_builder->widget_append_text_start(_ram);
			_builder->widget_append_text(_ram, stat_ram);
			_builder->widget_append_text(_ram, " mb");

			_builder->widget_append_text_start(_vram);
			_builder->widget_append_text(_vram, stat_vram);
			_builder->widget_append_text(_vram, " mb");

			_builder->widget_append_text_start(_vram_txt);
			_builder->widget_append_text(_vram_txt, stat_vram_txt);
			_builder->widget_append_text(_vram_txt, " mb");

			_builder->widget_append_text_start(_vram_res);
			_builder->widget_append_text(_vram_res, stat_vram_res);
			_builder->widget_append_text(_vram_res, " mb");
		}

		const float ms = static_cast<float>(frame_info::get_main_thread_time_milli());
		mem_fetch_time += ms;
		stat_fetch_time += ms;
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

		if (widget == ptr->_button_new_level)
		{
			editor::get().new_level();
			return vekt::input_event_result::handled;
		}

		if (widget == ptr->_button_load_level)
		{
			editor::get().load_level_prompt();
			return vekt::input_event_result::handled;
		}

		if (widget == ptr->_button_save_level)
		{
			editor::get().save_lavel();
			return vekt::input_event_result::handled;
		}

		return vekt::input_event_result::not_handled;
	}
}
