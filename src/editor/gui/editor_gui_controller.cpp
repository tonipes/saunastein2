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

#include "editor_gui_controller.hpp"
#include "editor_panel_stats.hpp"
#include "editor_panel_entities.hpp"
#include "editor_panel_inspector.hpp"
#include "editor_panels_world_view.hpp"
#include "editor_panel_world.hpp"
#include "editor/editor_layout.hpp"
#include "editor/editor.hpp"
#include "editor/editor_theme.hpp"

#include "gfx/renderer.hpp"
#include "game/game_world_renderer.hpp"
#include "app/debug_controller.hpp"
#include "data/char_util.hpp"
#include "common/system_info.hpp"
#include "app/app.hpp"
#include "platform/window.hpp"
#include "math/math.hpp"
#include "input/input_mappings.hpp"

#include "gui/icon_defs.hpp"
#include "gui/vekt.hpp"

namespace SFG
{
#define MAX_PAYLOAD_SIZE 128
	static constexpr float SEPARATOR_WIDTH	   = 4.0f;
	static constexpr float ENTITIES_MIN_WIDTH  = 160.0f;
	static constexpr float WORLD_MIN_WIDTH	   = 240.0f;
	static constexpr float INSPECTOR_MIN_WIDTH = 240.0f;

	void editor_gui_controller::init(vekt::builder* b)
	{
		_builder		   = b;
		_split_ratio	   = editor_layout::get().entities_world_split;
		_split_ratio_right = editor_layout::get().world_inspector_split;

		// Construct panels
		_panel_entities	 = new editor_panel_entities();
		_panel_stats	 = new editor_panel_stats();
		_panel_world	 = new editor_panel_world();
		_panel_inspector = new editor_panel_inspector();

		_layout_root = _builder->allocate();
		{
			vekt::pos_props& pp = _builder->widget_get_pos_props(_layout_root);
			pp.flags			= vekt::pos_flags::pf_x_abs | vekt::pos_flags::pf_y_abs | vekt::pos_flags::pf_child_pos_row;

			vekt::size_props& sz = _builder->widget_get_size_props(_layout_root);
			sz.flags			 = vekt::size_flags::sf_x_abs | vekt::size_flags::sf_y_abs;
			sz.child_margins	 = {0.0f, 0.0f, 0.0f, 0.0f};
			sz.spacing			 = 0.0f;

			// vekt::widget_gfx& gfx = _builder->widget_get_gfx(_layout_root);
			//  gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
			//  gfx.color			  = vector4(1, 0, 0, 1);
			_builder->widget_add_child(_builder->get_root(), _layout_root);
		}

		_panel_entities->init(_builder);
		{
			const vekt::id ent_root = _panel_entities->get_root();

			vekt::pos_props& pp = _builder->widget_get_pos_props(ent_root);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_child_pos_column;
			pp.pos.y			= 0.0f;

			vekt::size_props& sz = _builder->widget_get_size_props(ent_root);
			sz.flags			 = vekt::size_flags::sf_x_abs | vekt::size_flags::sf_y_relative;
			sz.size.x			 = _split_px;
			sz.size.y			 = 1.0f;

			_builder->widget_add_child(_layout_root, ent_root);
		}

		_layout_separator_left = _builder->allocate();
		{
			_builder->widget_add_child(_layout_root, _layout_separator_left);

			vekt::pos_props& pp = _builder->widget_get_pos_props(_layout_separator_left);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.y			= 0.5f;

			vekt::size_props& sz = _builder->widget_get_size_props(_layout_separator_left);
			sz.flags			 = vekt::size_flags::sf_x_abs | vekt::size_flags::sf_y_relative;
			sz.size.x			 = SEPARATOR_WIDTH;
			sz.size.y			 = 1.0f;

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(_layout_separator_left);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
			gfx.color			  = editor_theme::get().col_frame_bg;

			vekt::hover_callback& hb = _builder->widget_get_hover_callbacks(_layout_separator_left);
			hb.on_hover_begin		 = on_separator_hover_begin;
			hb.on_hover_end			 = on_separator_hover_end;

			vekt::mouse_callback& mc								   = _builder->widget_get_mouse_callbacks(_layout_separator_left);
			mc.on_drag												   = on_separator_drag;
			_builder->widget_get_user_data(_layout_separator_left).ptr = this;
		}

		_panel_world->init(_builder);
		{
			const vekt::id w_root = _panel_world->get_root();
			_builder->widget_add_child(_layout_root, w_root);

			vekt::pos_props& pp = _builder->widget_get_pos_props(w_root);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_child_pos_column;
			pp.pos.y			= 0.0f;

			vekt::size_props& sz = _builder->widget_get_size_props(w_root);
			sz.flags			 = vekt::size_flags::sf_x_abs | vekt::size_flags::sf_y_relative;
			sz.size.x			 = _split_world_px;
			sz.size.y			 = 1.0f;
		}

		_layout_separator_right = _builder->allocate();
		{
			_builder->widget_add_child(_layout_root, _layout_separator_right);

			vekt::pos_props& pp = _builder->widget_get_pos_props(_layout_separator_right);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.y			= 0.5f;

			vekt::size_props& sz = _builder->widget_get_size_props(_layout_separator_right);
			sz.flags			 = vekt::size_flags::sf_x_abs | vekt::size_flags::sf_y_relative;
			sz.size.x			 = SEPARATOR_WIDTH;
			sz.size.y			 = 1.0f;

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(_layout_separator_right);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
			gfx.color			  = editor_theme::get().col_frame_bg;

			vekt::hover_callback& hb = _builder->widget_get_hover_callbacks(_layout_separator_right);
			hb.on_hover_begin		 = on_separator_hover_begin;
			hb.on_hover_end			 = on_separator_hover_end;

			vekt::mouse_callback& mc									= _builder->widget_get_mouse_callbacks(_layout_separator_right);
			mc.on_drag													= on_separator_drag;
			_builder->widget_get_user_data(_layout_separator_right).ptr = this;
		}

		_panel_inspector->init(_builder, _panel_entities);
		{
			const vekt::id i_root = _panel_inspector->get_root();
			_builder->widget_add_child(_layout_root, i_root);

			vekt::pos_props& pp = _builder->widget_get_pos_props(i_root);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_child_pos_column;
			pp.pos.y			= 0.0f;

			vekt::size_props& sz = _builder->widget_get_size_props(i_root);
			sz.flags			 = vekt::size_flags::sf_x_fill | vekt::size_flags::sf_y_relative;
			sz.size.y			 = 1.0f;
		}

		_panel_stats->init(_builder);
		{
			const vekt::id stats_root = _panel_stats->get_root();

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(stats_root);
			gfx.flags			  = 0;

			vekt::pos_props& pp = _builder->widget_get_pos_props(stats_root);
			pp.flags			= vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_y_relative;
			pp.pos.x			= 0.0f;
			pp.pos.y			= 0.0f;

			vekt::size_props& sz = _builder->widget_get_size_props(stats_root);
			sz.flags			 = vekt::size_flags::sf_x_relative | vekt::size_flags::sf_y_relative;
			sz.size.x			 = 1.0f;
			sz.size.y			 = 1.0f;
			sz.child_margins	 = {};

			_builder->widget_add_child(_panel_world->get_game_view(), stats_root);
		}

		_payload = _builder->allocate();
		{
			vekt::pos_props& pp = _builder->widget_get_pos_props(_payload);
			pp.flags			= vekt::pos_flags::pf_x_abs | vekt::pos_flags::pf_y_abs;

			vekt::size_props& sz = _builder->widget_get_size_props(_payload);
			sz.flags			 = vekt::size_flags::sf_x_max_children | vekt::size_flags::sf_y_abs;
			sz.size.y			 = editor_theme::get().item_height;
			sz.child_margins	 = {0.0f, 0.0f, editor_theme::get().outer_margin, editor_theme::get().outer_margin};

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(_payload);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect | vekt::gfx_flags::gfx_has_stroke;
			gfx.color			  = editor_theme::get().col_area_bg;

			vekt::stroke_props& sp = _builder->widget_get_stroke(_payload);
			sp.color			   = editor_theme::get().col_frame_outline;
			sp.thickness		   = editor_theme::get().frame_thickness;
		}

		_payload_text = _builder->allocate();
		{
			vekt::pos_props& pp = _builder->widget_get_pos_props(_payload_text);
			pp.flags			= vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.x			= 0.0f;
			pp.pos.y			= 0.5f;

			vekt::text_props& tp = _builder->widget_get_text(_payload_text);
			tp.font				 = editor_theme::get().font_default;
			tp.text				 = editor::get().get_text_allocator().allocate(MAX_PAYLOAD_SIZE);
			_builder->widget_update_text(_payload_text);
		}

		_builder->widget_add_child(_builder->get_root(), _payload);
		_builder->widget_add_child(_payload, _payload_text);
		_builder->widget_set_visible(_payload, false);

		_panel_world->fetch_stats();
		_builder->build_hierarchy();
	}

	void editor_gui_controller::uninit()
	{
		editor::get().get_text_allocator().deallocate(_builder->widget_get_text(_payload_text).text);
		_builder->deallocate(_payload);
		_payload = _payload_text = NULL_WIDGET_ID;

		// Controls are disabled.

		_panel_entities->uninit();
		delete _panel_entities;
		_panel_entities = nullptr;

		_panel_world->uninit();
		delete _panel_world;
		_panel_world = nullptr;

		_panel_inspector->uninit();
		delete _panel_inspector;
		_panel_inspector = nullptr;

		_panel_stats->uninit();
		delete _panel_stats;
		_panel_stats = nullptr;
	}

	void editor_gui_controller::tick(world& w, const vector2ui16& window_size)
	{
		if (_payload_active)
		{
			const vector2i16 mp = editor::get().get_app().get_main_window().get_mouse_position();
			_builder->widget_set_pos_abs(_payload, vector2(mp.x + 20, mp.y + 20));
		}

		if (_ctx_active != NULL_WIDGET_ID)
		{
			const vector2 ctx_size = _builder->widget_get_size(_ctx_active);
			const vector2 ctx_pos  = _builder->widget_get_pos(_ctx_active);
			if (ctx_pos.x + ctx_size.x > window_size.x)
			{
				vekt::pos_props& pp = _builder->widget_get_pos_props(_ctx_active);
				pp.pos.x			= window_size.x - ctx_size.x - 50;
			}
		}

		_builder->widget_set_pos_abs(_layout_root, vector2(0.0f, debug_controller::get_field_height()));
		_builder->widget_set_size_abs(_layout_root, vector2(static_cast<float>(window_size.x), static_cast<float>(window_size.y) - debug_controller::get_field_height()));
		{
			const vector2 layout_size = _builder->widget_get_size(_layout_root);
			if (layout_size.x > 0.0f)
			{
				const float available	  = layout_size.x - (SEPARATOR_WIDTH * 2.0f);
				const float min_remaining = WORLD_MIN_WIDTH + INSPECTOR_MIN_WIDTH;
				const float max_width	  = math::max(available - min_remaining, ENTITIES_MIN_WIDTH);
				_split_px				  = math::clamp(layout_size.x * _split_ratio, ENTITIES_MIN_WIDTH, max_width);
				vekt::size_props& ent_sz  = _builder->widget_get_size_props(_panel_entities->get_root());
				ent_sz.size.x			  = _split_px;

				const float remaining	   = layout_size.x - _split_px - (SEPARATOR_WIDTH * 2.0f);
				const float max_world	   = math::max(remaining - INSPECTOR_MIN_WIDTH, WORLD_MIN_WIDTH);
				_split_world_px			   = math::clamp(remaining * _split_ratio_right, WORLD_MIN_WIDTH, max_world);
				vekt::size_props& world_sz = _builder->widget_get_size_props(_panel_world->get_root());
				world_sz.size.x			   = _split_world_px;
			}
		}
		_panel_entities->draw(w, window_size);
		_panel_world->draw(window_size);
		_panel_inspector->draw(w, window_size);
		_panel_stats->draw(window_size);

		editor_panel_entities* entities = _panel_entities;
		if (entities)
		{
			const world_handle	 selected		= entities->get_selected();
			const world_id		 selected_id	= selected.is_null() ? NULL_WORLD_ID : selected.index;
			game_world_renderer* world_renderer = editor::get().get_app().get_renderer().get_world_renderer();
			if (world_renderer)
				world_renderer->get_render_pass_selection_outline().set_selected_entity_id(selected_id);
		}
	}

	vekt::id editor_gui_controller::begin_context_menu(float abs_x, float abs_y)
	{
		if (_ctx_active != NULL_WIDGET_ID)
		{
			_builder->deallocate(_ctx_active);
			_ctx_active = _ctx_root = NULL_WIDGET_ID;
		}

		using namespace vekt;
		const editor_theme& theme = editor_theme::get();
		const id			w	  = _builder->allocate();
		_ctx_root				  = w;
		_builder->widget_add_child(_builder->get_root(), w);
		{
			pos_props& p = _builder->widget_get_pos_props(w);
			p.flags		 = pos_flags::pf_x_abs | pos_flags::pf_y_abs | pos_flags::pf_child_pos_column;
			p.pos.x		 = abs_x;
			p.pos.y		 = abs_y;

			size_props& sz	 = _builder->widget_get_size_props(w);
			sz.flags		 = size_flags::sf_x_max_children | size_flags::sf_y_total_children;
			sz.child_margins = {theme.inner_margin, theme.inner_margin, 0.0f, 0.0f};
			sz.spacing		 = theme.item_spacing;

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_stroke | gfx_flags::gfx_has_hover_color;
			gfx.color		= theme.col_area_bg;
			gfx.draw_order	= 1000;

			input_color_props& ip = _builder->widget_get_input_colors(w);
			ip.hovered_color	  = gfx.color;

			stroke_props& sp = _builder->widget_get_stroke(w);
			sp.thickness	 = theme.context_menu_outline_thickness;
			sp.color		 = theme.col_context_menu_outline;
		}

		return w;
	}

	vekt::id editor_gui_controller::add_context_menu_title(const char* label)
	{
		const editor_theme& theme = editor_theme::get();
		const vekt::id		w	  = _builder->allocate();
		_builder->widget_add_child(_ctx_root, w);
		{
			vekt::pos_props& p = _builder->widget_get_pos_props(w);
			p.flags			   = vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_child_pos_row;
			p.pos.x			   = 0.0f;

			vekt::size_props& sz = _builder->widget_get_size_props(w);
			sz.flags			 = vekt::size_flags::sf_x_abs | vekt::size_flags::sf_y_abs;
			sz.size.y			 = theme.item_height;
			sz.size.x			 = theme.item_height * 10;
			sz.child_margins	 = {0.0f, 0.0f, theme.inner_margin, theme.inner_margin};
			sz.spacing			 = theme.row_spacing;

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
			gfx.color			  = {0.0f, 0.0f, 0.0f, 0.0f};
			gfx.draw_order		  = 1001;
		}

		const vekt::id txt = _builder->allocate();
		{
			vekt::widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags			  = vekt::gfx_flags::gfx_is_text;
			gfx.color			  = theme.col_text_dim;
			gfx.draw_order		  = 1002;

			vekt::pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.y			= 0.5f;

			vekt::text_props& tp = _builder->widget_get_text(txt);
			tp.font				 = theme.font_default;
			_builder->widget_set_text(txt, label);
			_builder->widget_add_child(w, txt);
		}

		return w;
	}

	vekt::id editor_gui_controller::add_context_menu_item(const char* label)
	{
		const editor_theme& theme = editor_theme::get();
		const vekt::id		w	  = _builder->allocate();
		_builder->widget_add_child(_ctx_root, w);
		{
			vekt::pos_props& p = _builder->widget_get_pos_props(w);
			p.flags			   = vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_child_pos_row;
			p.pos.x			   = 0.0f;

			vekt::size_props& sz = _builder->widget_get_size_props(w);
			sz.flags			 = vekt::size_flags::sf_x_abs | vekt::size_flags::sf_y_abs;
			sz.size.y			 = theme.item_height;
			sz.size.x			 = theme.item_height * 10;
			sz.child_margins	 = {0.0f, 0.0f, theme.inner_margin, theme.inner_margin};
			sz.spacing			 = theme.row_spacing;

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect | vekt::gfx_flags::gfx_has_hover_color;
			gfx.color			  = {0.0f, 0.0f, 0.0f, 0.0f};
			gfx.draw_order		  = 1001;

			vekt::input_color_props& ip = _builder->widget_get_input_colors(w);
			ip.hovered_color			= editor_theme::get().col_accent_second;
		}

		const vekt::id empty = _builder->allocate();
		{
			vekt::widget_gfx& gfx = _builder->widget_get_gfx(empty);

			vekt::pos_props& pp = _builder->widget_get_pos_props(empty);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.y			= 0.5f;

			vekt::size_props& sz = _builder->widget_get_size_props(empty);
			sz.flags			 = vekt::size_flags::sf_y_relative | vekt::size_flags::sf_x_copy_y;
			sz.size.y			 = 0.5f;
			_builder->widget_add_child(w, empty);
		}

		const vekt::id txt = _builder->allocate();
		{
			vekt::widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags			  = vekt::gfx_flags::gfx_is_text;
			gfx.color			  = theme.col_text;
			gfx.draw_order		  = 1002;

			vekt::pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.y			= 0.5f;

			vekt::text_props& tp = _builder->widget_get_text(txt);
			tp.font				 = theme.font_default;
			_builder->widget_set_text(txt, label);
			_builder->widget_add_child(w, txt);
		}

		return w;
	}

	vekt::id editor_gui_controller::add_context_menu_item_toggle(const char* label, bool is_toggled)
	{
		const editor_theme& theme = editor_theme::get();
		const vekt::id		w	  = _builder->allocate();
		_builder->widget_add_child(_ctx_root, w);
		{
			vekt::pos_props& p = _builder->widget_get_pos_props(w);
			p.flags			   = vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_child_pos_row;
			p.pos.x			   = 0.0f;

			vekt::size_props& sz = _builder->widget_get_size_props(w);
			sz.flags			 = vekt::size_flags::sf_x_abs | vekt::size_flags::sf_y_abs;
			sz.size.y			 = theme.item_height;
			sz.size.x			 = theme.item_height * 10;
			sz.child_margins	 = {0.0f, 0.0f, theme.inner_margin, theme.inner_margin};
			sz.spacing			 = theme.row_spacing;

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect | vekt::gfx_flags::gfx_has_hover_color;
			gfx.color			  = {0.0f, 0.0f, 0.0f, 0.0f};
			gfx.draw_order		  = 1001;

			vekt::input_color_props& ip = _builder->widget_get_input_colors(w);
			ip.hovered_color			= editor_theme::get().col_accent_second;
		}

		const vekt::id icn = _builder->allocate();
		{
			vekt::widget_gfx& gfx = _builder->widget_get_gfx(icn);
			gfx.flags			  = vekt::gfx_flags::gfx_is_text;
			gfx.color			  = is_toggled ? theme.col_accent : vector4();
			gfx.draw_order		  = 1002;

			vekt::pos_props& pp = _builder->widget_get_pos_props(icn);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.y			= 0.5f;

			vekt::text_props& tp = _builder->widget_get_text(icn);
			tp.font				 = theme.font_icons;
			_builder->widget_set_text(icn, ICON_FILLED_CIRCLE);
			_builder->widget_add_child(w, icn);
		}

		const vekt::id txt = _builder->allocate();
		{
			vekt::widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags			  = vekt::gfx_flags::gfx_is_text;
			gfx.color			  = theme.col_text;
			gfx.draw_order		  = 1002;

			vekt::pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags			= vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.y			= 0.5f;

			vekt::text_props& tp = _builder->widget_get_text(txt);
			tp.font				 = theme.font_default;
			_builder->widget_set_text(txt, label);
			_builder->widget_add_child(w, txt);
		}

		return w;
	}

	void editor_gui_controller::end_context_menu()
	{
		_ctx_active = _ctx_root;
		_ctx_root	= NULL_WIDGET_ID;
		_ctx_frame	= frame_info::get_frame();
		_builder->build_hierarchy();
	}

	void editor_gui_controller::enable_payload(const char* text)
	{
		SFG_ASSERT(strlen(text) < MAX_PAYLOAD_SIZE);
		vekt::text_props& tp  = _builder->widget_get_text(_payload_text);
		char*			  cur = (char*)tp.text;
		char_util::append(cur, cur + MAX_PAYLOAD_SIZE, text);
		_builder->widget_update_text(_payload_text);
		_builder->widget_set_visible(_payload, true);
		_payload_active = 1;
	}

	void editor_gui_controller::disable_payload()
	{
		_builder->widget_set_visible(_payload, false);
		_payload_active = 0;
	}

	void editor_gui_controller::on_separator_hover_begin(vekt::builder* b, vekt::id widget)
	{
		SFG::window::set_cursor_state(SFG::cursor_state::resize_hr);
	}

	void editor_gui_controller::on_separator_hover_end(vekt::builder* b, vekt::id widget)
	{
		SFG::window::set_cursor_state(SFG::cursor_state::arrow);
	}

	void editor_gui_controller::on_separator_drag(vekt::builder* b, vekt::id widget, float mp_x, float mp_y, float delta_x, float delta_y, unsigned int button)
	{
		if (button != static_cast<uint16>(input_code::mouse_0))
			return;

		editor_gui_controller* self = static_cast<editor_gui_controller*>(b->widget_get_user_data(widget).ptr);
		if (!self)
			return;

		const vector2 layout_size = b->widget_get_size(self->_layout_root);
		if (widget == self->_layout_separator_left)
		{
			const float available	  = layout_size.x - (SEPARATOR_WIDTH * 2.0f);
			const float min_remaining = WORLD_MIN_WIDTH + INSPECTOR_MIN_WIDTH;
			const float max_width	  = math::max(available - min_remaining, ENTITIES_MIN_WIDTH);

			self->_split_px = math::clamp(self->_split_px + delta_x, ENTITIES_MIN_WIDTH, max_width);
			if (layout_size.x > 0.0f)
			{
				self->_split_ratio						  = math::clamp(self->_split_px / layout_size.x, 0.0f, 1.0f);
				editor_layout::get().entities_world_split = self->_split_ratio;
			}

			const vekt::id	  ent_root = self->_panel_entities->get_root();
			vekt::size_props& ent_sz   = b->widget_get_size_props(ent_root);
			ent_sz.size.x			   = self->_split_px;

			const float remaining	   = layout_size.x - self->_split_px - (SEPARATOR_WIDTH * 2.0f);
			const float max_world	   = math::max(remaining - INSPECTOR_MIN_WIDTH, WORLD_MIN_WIDTH);
			self->_split_world_px	   = math::clamp(remaining * self->_split_ratio_right, WORLD_MIN_WIDTH, max_world);
			vekt::size_props& world_sz = b->widget_get_size_props(self->_panel_world->get_root());
			world_sz.size.x			   = self->_split_world_px;
		}
		else if (widget == self->_layout_separator_right)
		{
			const float remaining = layout_size.x - self->_split_px - (SEPARATOR_WIDTH * 2.0f);
			const float max_world = math::max(remaining - INSPECTOR_MIN_WIDTH, WORLD_MIN_WIDTH);

			self->_split_world_px = math::clamp(self->_split_world_px + delta_x, WORLD_MIN_WIDTH, max_world);
			if (remaining > 0.0f)
			{
				self->_split_ratio_right				   = math::clamp(self->_split_world_px / remaining, 0.0f, 1.0f);
				editor_layout::get().world_inspector_split = self->_split_ratio_right;
			}

			vekt::size_props& world_sz = b->widget_get_size_props(self->_panel_world->get_root());
			world_sz.size.x			   = self->_split_world_px;
		}
	}

	bool editor_gui_controller::on_mouse_event(const window_event& ev)
	{
		if (ev.type == window_event_type::mouse)
		{
			const vekt::input_event_result res = _builder->on_mouse_event({
				.type	  = static_cast<vekt::input_event_type>(ev.sub_type),
				.button	  = ev.button,
				.position = VEKT_VEC2(ev.value.x, ev.value.y),
			});

			if (frame_info::get_frame() != _ctx_frame && _ctx_active != NULL_WIDGET_ID && ev.sub_type == window_event_sub_type::press)
			{
				_panel_world->kill_context();
				_panel_entities->kill_context();
				_panel_inspector->kill_context();
				_builder->deallocate(_ctx_active);
				_ctx_active = _ctx_root = NULL_WIDGET_ID;
			}

			if (_panel_world && _panel_world->on_mouse_event(ev))
				return true;

			if (_builder->widget_get_hover_callbacks(_panel_entities->get_root()).is_hovered)
				return true;

			if (_builder->widget_get_hover_callbacks(_panel_inspector->get_root()).is_hovered)
				return true;
		}

		if (_payload_active && ev.sub_type == window_event_sub_type::release)
		{
			_panel_entities->kill_drag();
			disable_payload();
		}

		return false;
	}

	bool editor_gui_controller::on_mouse_move(const vector2& p)
	{
		return _panel_world->on_mouse_move(p);
	}

	bool editor_gui_controller::on_key_event(const window_event& ev)
	{
		if (_panel_world->on_key_event(ev))
			return true;

		return false;
	}

	vector2 editor_gui_controller::get_world_size()
	{
		return _panel_world->get_world_size();
	}
	void editor_gui_controller::on_exited_playmode()
	{
		_panel_world->kill_playmode();
	}
}
