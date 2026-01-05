/*
/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

See root license for details.
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

#include "common/system_info.hpp"
#include "app/app.hpp"

#include "gui/vekt.hpp"

namespace SFG
{
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
	}

	void editor_gui_controller::uninit()
	{
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

		_panel_controls->draw(window_size);
		_panel_entities->draw(w, window_size);
	}

	vekt::id editor_gui_controller::begin_context_menu(float abs_x, float abs_y)
	{
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

			_builder->widget_get_user_data(w).ptr = this;
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
			tp.text		   = editor::get().get_text_allocator().allocate(label);
			tp.font		   = theme.font_default;
			_builder->widget_update_text(txt);
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

	void editor_gui_controller::on_context_item_hover_begin(vekt::builder* b, vekt::id widget)
	{
		b->widget_get_gfx(widget).color = editor_theme::get().col_highlight_transparent;
	}
	void editor_gui_controller::on_context_item_hover_end(vekt::builder* b, vekt::id widget)
	{
		b->widget_get_gfx(widget).color = {};
	}

	void editor_gui_controller::set_entities_tree_dirty()
	{
		if (_panel_entities)
			_panel_entities->set_tree_dirty();
	}
	void editor_gui_controller::on_mouse_event(const window_event& ev)
	{
		if (_ctx_active == NULL_WIDGET_ID)
			return;

		if (ev.sub_type != window_event_sub_type::press)
			return;

		if (frame_info::get_frame() == _ctx_frame)
			return;

		_builder->deallocate(_ctx_active);
		_ctx_active = _ctx_root = NULL_WIDGET_ID;
	}
}
