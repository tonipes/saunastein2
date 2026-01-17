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

#include "editor_panels_world_view.hpp"
#include "math/vector2ui16.hpp"
#include "editor/editor.hpp"
#include "editor/editor_settings.hpp"
#include "editor/editor_theme.hpp"
#include "app/app.hpp"
#include "gfx/backend/backend.hpp"
#include "gui/vekt.hpp"
#include "gui/icon_defs.hpp"
#include "platform/process.hpp"
#include "memory/memory_tracer.hpp"
#include "common/system_info.hpp"

namespace SFG
{
	void editor_panels_world_view::init(vekt::builder* b)
	{
		_builder = b;
		_gui_builder.init(b);
		_root = _gui_builder.get_root();

		vekt::widget_gfx& gfx = _builder->widget_get_gfx(_root);
		gfx.user_data		  = &_user_data;

		_user_data.type = editor_gui_user_data_type::world_rt;

		_gui_builder.callbacks.user_data   = this;
		_gui_builder.callbacks.callback_ud = this;
		_gui_builder.callbacks.on_mouse	   = on_mouse;

		// Create a top-right overlay column and add icon buttons
		_icon_column = _builder->allocate();
		{
			vekt::pos_props& pp = _builder->widget_get_pos_props(_icon_column);
			pp.flags			= vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_x_anchor_end | vekt::pos_flags::pf_child_pos_column;
			pp.pos				= {1.0f, 0.0f};

			vekt::size_props& sz = _builder->widget_get_size_props(_icon_column);
			sz.flags			 = vekt::size_flags::sf_x_max_children | vekt::size_flags::sf_y_total_children;
			sz.spacing			 = editor_theme::get().row_spacing;
			sz.child_margins	 = {editor_theme::get().inner_margin, editor_theme::get().inner_margin, editor_theme::get().inner_margin, editor_theme::get().inner_margin};

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(_icon_column);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
			gfx.color			  = editor_theme::get().col_root;
			gfx.draw_order		  = 1;

			_builder->widget_get_user_data(_icon_column).ptr = this;
		}
		_builder->widget_add_child(_root, _icon_column);

		_gui_builder.push_stack(_icon_column);
		_gui_builder.set_draw_order(1);

		_btn_view  = _gui_builder.add_icon_button(ICON_EYE, 0, 1.5f, false).first;
		_btn_menu  = _gui_builder.add_icon_button(ICON_FILE, 0, 1.5f, false).first;
		_btn_stats = _gui_builder.add_icon_button(ICON_INFO, 0, 1.5f, false).first;
		_btn_play  = _gui_builder.add_icon_button(ICON_PLAY, 0, 1.5f, false, editor_theme::get().col_accent_third).first;

		{
			vekt::pos_props& pp = _builder->widget_get_pos_props(_btn_menu);
			pp.flags			= vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_x_anchor_center;
			pp.pos.x			= 0.5f;
		}

		{
			vekt::pos_props& pp = _builder->widget_get_pos_props(_btn_stats);
			pp.flags			= vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_x_anchor_center;
			pp.pos.x			= 0.5f;
		}
		_gui_builder.set_draw_order(0);

		_gui_builder.pop_stack();
	}

	void editor_panels_world_view::uninit()
	{
		_gui_builder.uninit();
	}

	void editor_panels_world_view::draw(const vector2ui16& window_size)
	{
		if (_stats_area == NULL_WIDGET_ID)
			return;

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

			_builder->widget_append_text_start(_wv_game_res);
			_builder->widget_append_text(_wv_game_res, game_size.x, 0);
			_builder->widget_append_text(_wv_game_res, "x");
			_builder->widget_append_text(_wv_game_res, game_size.y, 0);

			_builder->widget_append_text_start(_wv_window_res);
			_builder->widget_append_text(_wv_window_res, window_size.x, 0);
			_builder->widget_append_text(_wv_window_res, "x");
			_builder->widget_append_text(_wv_window_res, window_size.y, 0);

			_builder->widget_append_text_start(_wv_fps);
			_builder->widget_append_text(_wv_fps, stat_fps);

			_builder->widget_append_text_start(_wv_main);
			_builder->widget_append_text(_wv_main, stat_main_thread);
			_builder->widget_append_text(_wv_main, " ms");

			_builder->widget_append_text_start(_wv_render);
			_builder->widget_append_text(_wv_render, stat_render_thread);
			_builder->widget_append_text(_wv_render, " ms");

			_builder->widget_append_text_start(_wv_loaded_project);
			_builder->widget_append_text(_wv_loaded_project, editor_settings::get().working_dir.c_str());

			_builder->widget_append_text_start(_wv_loaded_level);
			_builder->widget_append_text(_wv_loaded_level, editor::get().get_loaded_level().c_str());

			_builder->widget_append_text_start(_wv_draw_calls);
			_builder->widget_append_text(_wv_draw_calls, stat_dc);
		}

		if (mem_fetch_time > 6000)
		{
#ifdef SFG_ENABLE_MEMORY_TRACER
			memory_tracer& tracer = memory_tracer::get();
			LOCK_GUARD(tracer.get_category_mtx());

			for (const memory_category& cat : tracer.get_categories())
			{
				if (TO_SID(cat.name) == TO_SID("General"))
					stat_ram = static_cast<uint32>(static_cast<float>(cat.total_size) / (1024 * 1024));
				else if (TO_SID(cat.name) == TO_SID("Gfx"))
					stat_vram = static_cast<uint32>(static_cast<float>(cat.total_size) / (1024 * 1024));
				else if (TO_SID(cat.name) == TO_SID("GfxTxt"))
					stat_vram_txt = static_cast<uint32>(static_cast<float>(cat.total_size) / (1024 * 1024));
				else if (TO_SID(cat.name) == TO_SID("GfxRes"))
					stat_vram_res = static_cast<uint32>(static_cast<float>(cat.total_size) / (1024 * 1024));
			}
#endif
			mem_fetch_time = 0.0f;

			_builder->widget_append_text_start(_wv_ram);
			_builder->widget_append_text(_wv_ram, stat_ram);
			_builder->widget_append_text(_wv_ram, " mb");

			_builder->widget_append_text_start(_wv_vram);
			_builder->widget_append_text(_wv_vram, stat_vram);
			_builder->widget_append_text(_wv_vram, " mb");

			_builder->widget_append_text_start(_wv_vram_txt);
			_builder->widget_append_text(_wv_vram_txt, stat_vram_txt);
			_builder->widget_append_text(_wv_vram_txt, " mb");

			_builder->widget_append_text_start(_wv_vram_res);
			_builder->widget_append_text(_wv_vram_res, stat_vram_res);
			_builder->widget_append_text(_wv_vram_res, " mb");
		}

		const float ms = static_cast<float>(frame_info::get_main_thread_time_milli());
		mem_fetch_time += ms;
		stat_fetch_time += ms;
	}

	vekt::input_event_result editor_panels_world_view::on_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		if (ev.type != vekt::input_event_type::pressed)
			return vekt::input_event_result::not_handled;

		editor_panels_world_view* self = static_cast<editor_panels_world_view*>(b->widget_get_user_data(widget).ptr);

		if (widget == self->_btn_view)
		{
			editor::get().get_gui_controller().begin_context_menu(ev.position.x, ev.position.y);

			self->_ctx_world_rt	   = editor::get().get_gui_controller().add_context_menu_item("world");
			self->_ctx_colors_rt   = editor::get().get_gui_controller().add_context_menu_item("gbuffer_color");
			self->_ctx_normals_rt  = editor::get().get_gui_controller().add_context_menu_item("gbuffer_normal");
			self->_ctx_orm_rt	   = editor::get().get_gui_controller().add_context_menu_item("gbuffer_orm");
			self->_ctx_emissive_rt = editor::get().get_gui_controller().add_context_menu_item("gbuffer_emissive");
			self->_ctx_depth_rt	   = editor::get().get_gui_controller().add_context_menu_item("depth");
			self->_ctx_lighting_rt = editor::get().get_gui_controller().add_context_menu_item("lighting");
			self->_ctx_ssao_rt	   = editor::get().get_gui_controller().add_context_menu_item("ssao");
			self->_ctx_bloom_rt	   = editor::get().get_gui_controller().add_context_menu_item("bloom");

			auto hook = [&](vekt::id id) {
				vekt::mouse_callback& cb		= b->widget_get_mouse_callbacks(id);
				cb.on_mouse						= on_mouse;
				b->widget_get_user_data(id).ptr = self;
			};

			hook(self->_ctx_world_rt);
			hook(self->_ctx_colors_rt);
			hook(self->_ctx_normals_rt);
			hook(self->_ctx_orm_rt);
			hook(self->_ctx_emissive_rt);
			hook(self->_ctx_depth_rt);
			hook(self->_ctx_lighting_rt);
			hook(self->_ctx_ssao_rt);
			hook(self->_ctx_bloom_rt);

			editor::get().get_gui_controller().end_context_menu();
			return vekt::input_event_result::handled;
		}

		if (widget == self->_btn_menu)
		{
			editor::get().get_gui_controller().begin_context_menu(ev.position.x, ev.position.y);

			self->_ctx_new_project	   = editor::get().get_gui_controller().add_context_menu_item("new project");
			self->_ctx_open_project	   = editor::get().get_gui_controller().add_context_menu_item("open project");
			self->_ctx_package_project = editor::get().get_gui_controller().add_context_menu_item("package project");
			self->_ctx_new_world	   = editor::get().get_gui_controller().add_context_menu_item("new world");
			self->_ctx_save_world	   = editor::get().get_gui_controller().add_context_menu_item("save world");
			self->_ctx_open_world	   = editor::get().get_gui_controller().add_context_menu_item("open world");

			auto hook = [&](vekt::id id) {
				vekt::mouse_callback& cb		= b->widget_get_mouse_callbacks(id);
				cb.on_mouse						= on_mouse;
				b->widget_get_user_data(id).ptr = self;
			};

			hook(self->_ctx_new_project);
			hook(self->_ctx_open_project);
			hook(self->_ctx_package_project);
			hook(self->_ctx_new_world);
			hook(self->_ctx_save_world);
			hook(self->_ctx_open_world);

			editor::get().get_gui_controller().end_context_menu();
			return vekt::input_event_result::handled;
		}

		if (widget == self->_btn_stats)
		{
			if (self->_stats_area != NULL_WIDGET_ID)
			{
				self->_gui_builder.deallocate(self->_stats_area);
				self->_stats_area = NULL_WIDGET_ID;
				return vekt::input_event_result::handled;
			}
			self->_gui_builder.set_draw_order(2);

			self->_stats_area = self->_gui_builder.begin_area(false, true);

			{
				vekt::pos_props& pp = b->widget_get_pos_props(self->_stats_area);
				pp.flags			= vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_end | vekt::pos_flags::pf_x_anchor_end | vekt::pos_flags::pf_child_pos_column;
				pp.pos				= {1.0f, 1.0f};

				vekt::size_props& sz = b->widget_get_size_props(self->_stats_area);
				sz.flags			 = vekt::size_flags::sf_x_abs | vekt::size_flags::sf_y_abs;
				sz.spacing			 = editor_theme::get().item_spacing;
				sz.child_margins	 = {editor_theme::get().outer_margin, editor_theme::get().outer_margin, editor_theme::get().outer_margin, editor_theme::get().outer_margin};
				sz.size.x			 = 400;
				sz.size.y			 = 300;

				vekt::widget_gfx& gfx = b->widget_get_gfx(self->_stats_area);
				gfx.color.w			  = 0.75f;

				self->_gui_builder.set_draw_order(2);

				self->_wv_game_res		 = self->_gui_builder.add_property_row_label("game_res:", "fetching...", 12).second;
				self->_wv_window_res	 = self->_gui_builder.add_property_row_label("window_res:", "fetching...", 12).second;
				self->_wv_fps			 = self->_gui_builder.add_property_row_label("fps:", "fetching...", 12).second;
				self->_wv_main			 = self->_gui_builder.add_property_row_label("main:", "fetching...", 12).second;
				self->_wv_render		 = self->_gui_builder.add_property_row_label("render:", "fetching...", 12).second;
				self->_wv_ram			 = self->_gui_builder.add_property_row_label("ram:", "fetching...", 12).second;
				self->_wv_vram			 = self->_gui_builder.add_property_row_label("vram:", "fetching...", 12).second;
				self->_wv_vram_txt		 = self->_gui_builder.add_property_row_label("vram_texture:", "fetching...", 12).second;
				self->_wv_vram_res		 = self->_gui_builder.add_property_row_label("vram_buffer:", "fetching...", 12).second;
				self->_wv_draw_calls	 = self->_gui_builder.add_property_row_label("draw_calls:", "fetching...", 12).second;
				self->_wv_loaded_project = self->_gui_builder.add_property_row_label("project:", editor_settings::get().working_dir.c_str(), 1024).second;
				self->_wv_loaded_level	 = self->_gui_builder.add_property_row_label("world:", editor::get().get_loaded_level().c_str(), 1024).second;
			}
			self->_gui_builder.end_area();
			self->_gui_builder.set_draw_order(0);

			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_world_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::world_rt;
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_colors_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::colors_rt;
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_normals_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::normals_rt;
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_orm_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::orm_rt;
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_emissive_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::emissive_rt;
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_lighting_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::lighting_rt;
			return vekt::input_event_result::handled;
		}
		if (widget == self->_ctx_bloom_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::bloom_rt;
			return vekt::input_event_result::handled;
		}
		if (widget == self->_ctx_depth_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::depth_rt;
			return vekt::input_event_result::handled;
		}
		if (widget == self->_ctx_ssao_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::ssao_rt;
			return vekt::input_event_result::handled;
		}
		if (widget == self->_ctx_new_project)
		{
			const string dir = process::select_folder("select project directory");
			if (!dir.empty())
				editor::get().new_project(dir.c_str());
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_open_project)
		{
			process::open_directory(editor_settings::get().working_dir.c_str());
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_package_project)
		{
			// Not implemented in panel controls either; handle click.
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_new_world)
		{
			editor::get().new_level();
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_save_world)
		{
			editor::get().save_lavel();
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_open_world)
		{
			editor::get().load_level_prompt();
			return vekt::input_event_result::handled;
		}

		return vekt::input_event_result::not_handled;
	}

	bool editor_panels_world_view::consume_committed_size(vector2ui16& out_size)
	{
		return false;
	}
}
