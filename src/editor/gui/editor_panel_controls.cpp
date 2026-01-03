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

#include "app/app.hpp"

#include "gui/vekt.hpp"
#include "gui/vekt_gui_builder.hpp"

namespace SFG
{
	void editor_panel_controls::init(vekt::builder* builder)
	{
		return;
		vekt::gui_builder gui_builder	= vekt::gui_builder(builder);
		gui_builder.callbacks.on_mouse	= on_mouse;
		gui_builder.callbacks.user_data = this;
		gui_builder.style.title_font	= editor_theme::get().font_title;
		gui_builder.style.default_font	= editor_theme::get().font_default;

		_widget = gui_builder.begin_root();

		// general
		gui_builder.add_title("general");

		gui_builder.begin_area(false);
		const string vstr = "stakeforge_engine v." + std::to_string(SFG_MAJOR) + "." + std::to_string(SFG_MINOR) + "." + string(SFG_BUILD);
		gui_builder.add_property_single_label(vstr.c_str());
		_hyperlink = gui_builder.add_property_single_hyperlink("github");
		gui_builder.end_area();

		// stats
		gui_builder.add_title("stats");
		gui_builder.begin_area();

		_game_res	= gui_builder.add_property_row_label("game_res:", "-no data-").second;
		_window_res = gui_builder.add_property_row_label("window_res:", "-no data-").second;
		_fps		= gui_builder.add_property_row_label("fps:", "-no data-").second;
		_main		= gui_builder.add_property_row_label("main:", "-no data-").second;
		_render		= gui_builder.add_property_row_label("render:", "-no data-").second;
		_ram		= gui_builder.add_property_row_label("ram:", "-no data-").second;
		_vram		= gui_builder.add_property_row_label("vram:", "-no data-").second;
		_vram_txt	= gui_builder.add_property_row_label("vram_texture:", "-no data-").second;
		_vram_res	= gui_builder.add_property_row_label("vram_buffer:", "-no data-").second;
		_draw_calls = gui_builder.add_property_row_label("draw_calls:", "-no data-").second;
		gui_builder.end_area();

		gui_builder.end_root();

		builder->widget_add_child(builder->get_root(), _widget);
	}

	void editor_panel_controls::uninit(vekt::builder* b)
	{
		b->deallocate(_widget);
	}

	void editor_panel_controls::draw(const vector2ui16& window_size, vekt::builder* b)
	{
		return;

		b->widget_set_pos_abs(_widget, vector2(30, 60));
		b->widget_set_size_abs(_widget, vector2(700, 400));


		static float fetch_time = 0.0f;

		const float main_time_ms = static_cast<float>(frame_info::get_main_thread_time_milli());

		const uint32 dc = frame_info::get_draw_calls();

		if (fetch_time > 1500.0f)
		{
			size_t ram		= 0;
			size_t vram		= 0;
			size_t vram_txt = 0;
			size_t vram_res = 0;

#ifdef SFG_ENABLE_MEMORY_TRACER
			{
				memory_tracer& tracer = memory_tracer::get();
				LOCK_GUARD(tracer.get_category_mtx());

				for (const memory_category& cat : tracer.get_categories())
				{
					if (TO_SID(cat.name) == TO_SID("General"))
					{
						ram = static_cast<float>(cat.total_size) / (1024 * 1024);
					}
					else if (TO_SID(cat.name) == TO_SID("Gfx"))
					{
						vram = static_cast<float>(cat.total_size) / (1024 * 1024);
					}
					else if (TO_SID(cat.name) == TO_SID("GfxTxt"))
					{
						vram_txt = static_cast<float>(cat.total_size) / (1024 * 1024);
					}
					else if (TO_SID(cat.name) == TO_SID("GfxRes"))
					{
						vram_res = static_cast<float>(cat.total_size) / (1024 * 1024);
					}
				}
			}

#endif

			const vector2ui16& game_res = editor::get().get_app().get_game_resolution();

			b->widget_get_text(_game_res).text	 = std::to_string(game_res.x) + "x" + std::to_string(game_res.y);
			b->widget_get_text(_window_res).text = std::to_string(window_size.x) + "x" + std::to_string(window_size.y);
			b->widget_get_text(_fps).text		 = std::to_string(frame_info::get_fps());
			b->widget_get_text(_main).text		 = std::to_string(main_time_ms) + " ms";
			b->widget_get_text(_render).text	 = std::to_string(frame_info::get_render_thread_time_milli()) + " ms";
			b->widget_get_text(_ram).text		 = std::to_string(ram) + " mb";
			b->widget_get_text(_vram).text		 = std::to_string(vram) + " mb";
			b->widget_get_text(_vram_txt).text	 = std::to_string(vram_txt) + " mb";
			b->widget_get_text(_vram_res).text	 = std::to_string(vram_res) + " mb";
			b->widget_get_text(_draw_calls).text = std::to_string(dc);
			b->widget_update_text(_game_res);
			b->widget_update_text(_window_res);
			b->widget_update_text(_fps);
			b->widget_update_text(_main);
			b->widget_update_text(_render);
			b->widget_update_text(_ram);
			b->widget_update_text(_vram);
			b->widget_update_text(_vram_txt);
			b->widget_update_text(_vram_res);
			b->widget_update_text(_draw_calls);
			fetch_time = 0;
		}

		fetch_time += main_time_ms;
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
