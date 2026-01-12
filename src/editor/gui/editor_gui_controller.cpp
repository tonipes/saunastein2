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
#include "editor_panel_controls.hpp"
#include "editor_panel_entities.hpp"
#include "editor_panel_properties.hpp"
#include "editor_gui_world_overlays.hpp"
#include "editor_panels_docking.hpp"
#include "editor_panels_world_view.hpp"
#include "editor/editor.hpp"
#include "editor/editor_theme.hpp"

#include "data/char_util.hpp"
#include "common/system_info.hpp"
#include "app/app.hpp"
#include "platform/window.hpp"

#include "gui/vekt.hpp"

namespace SFG
{
#define MAX_PAYLOAD_SIZE 128

	void editor_gui_controller::init(vekt::builder* b)
	{
		_builder = b;

		// Construct panels
		_gui_world_overlays = new editor_gui_world_overlays();
		_panel_controls		= new editor_panel_controls();
		_panel_entities		= new editor_panel_entities();
		_panel_properties	= new editor_panel_properties();
		_panel_world_view	= new editor_panels_world_view();
		_panels_docking		= new editor_panels_docking();

		_gui_world_overlays->init(_builder);
		_panel_controls->init(_builder);
		_panel_entities->init(_builder);
		_panel_properties->init();
		_panel_world_view->init();
		_panels_docking->init();

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
	}

	void editor_gui_controller::uninit()
	{
		editor::get().get_text_allocator().deallocate(_builder->widget_get_text(_payload_text).text);
		_builder->deallocate(_payload);
		_payload = _payload_text = NULL_WIDGET_ID;

		_panel_controls->uninit();
		delete _panel_controls;
		_panel_controls = nullptr;

		_panel_entities->uninit();
		delete _panel_entities;
		_panel_entities = nullptr;

		_panel_world_view->uninit();
		delete _panel_world_view;
		_panel_world_view = nullptr;

		_panels_docking->uninit();
		delete _panels_docking;
		_panels_docking = nullptr;

		_panel_properties->uninit();
		delete _panel_properties;
		_panel_properties = nullptr;

		delete _gui_world_overlays;
		_gui_world_overlays = nullptr;
	}

	void editor_gui_controller::tick(world& w, const vector2ui16& window_size)
	{
		vector2ui16 world_res;
		if (_panel_world_view->consume_committed_size(world_res))
			editor::get().get_app().set_game_resolution(world_res);

		if (_payload_active)
		{
			const vector2i16 mp = editor::get().get_app().get_main_window().get_mouse_position();
			_builder->widget_set_pos_abs(_payload, vector2(mp.x + 20, mp.y + 20));
		}
		_panel_controls->draw(window_size);
		_panel_entities->draw(w, window_size);
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
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_stroke;
			gfx.color		= theme.col_area_bg;
			gfx.draw_order	= 1000;

			stroke_props& sp = _builder->widget_get_stroke(w);
			sp.thickness	 = theme.context_menu_outline_thickness;
			sp.color		 = theme.col_context_menu_outline;
		}
		return w;
	}

	vekt::id editor_gui_controller::add_context_menu_item(const char* label)
	{
		using namespace vekt;
		const editor_theme& theme = editor_theme::get();
		const id			w	  = _builder->allocate();
		_builder->widget_add_child(_ctx_root, w);
		{
			pos_props& p = _builder->widget_get_pos_props(w);
			p.flags		 = pos_flags::pf_x_relative | pos_flags::pf_child_pos_row;
			p.pos.x		 = 0.0f;

			size_props& sz	 = _builder->widget_get_size_props(w);
			sz.flags		 = size_flags::sf_x_abs | size_flags::sf_y_abs;
			sz.size.y		 = theme.item_height;
			sz.size.x		 = 200;
			sz.child_margins = {0.0f, 0.0f, theme.inner_margin, theme.inner_margin};
			sz.spacing		 = theme.row_spacing;

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect;
			gfx.color		= {0.0f, 0.0f, 0.0f, 0.0f};
			gfx.draw_order	= 1001;

			hover_callback& hb = _builder->widget_get_hover_callbacks(w);
			hb.on_hover_begin  = on_context_item_hover_begin;
			hb.on_hover_end	   = on_context_item_hover_end;
			hb.receive_mouse   = 1;
		}

		const id txt = _builder->allocate();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags		= gfx_flags::gfx_is_text;
			gfx.color		= theme.col_text;
			gfx.draw_order	= 1002;

			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
			pp.pos.y	  = 0.5f;

			text_props& tp = _builder->widget_get_text(txt);
			tp.font		   = theme.font_default;
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

	void editor_gui_controller::on_context_item_hover_begin(vekt::builder* b, vekt::id widget)
	{
		b->widget_get_gfx(widget).color = editor_theme::get().col_highlight_transparent;
	}
	void editor_gui_controller::on_context_item_hover_end(vekt::builder* b, vekt::id widget)
	{
		b->widget_get_gfx(widget).color = {};
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
				_panel_entities->kill_context();
				_builder->deallocate(_ctx_active);
				_ctx_active = _ctx_root = NULL_WIDGET_ID;
			}

			if (_builder->widget_get_hover_callbacks(_panel_entities->get_root()).is_hovered)
				return true;
		}

		if (_payload_active && ev.sub_type == window_event_sub_type::release)
		{
			_panel_entities->kill_drag();
			disable_payload();
		}

		return false;
	}
}
