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

#include "vekt_gui_builder.hpp"
#include "vekt.hpp"
#include "math/color.hpp"

#include "platform/window.hpp"

namespace vekt
{

	input_event_result on_mouse(builder* b, id widget, const mouse_event& ev, input_event_phase phase)
	{
		if (!b->widget_get_hover_callbacks(widget).is_hovered)
			return input_event_result::not_handled;

		return input_event_result::not_handled;
	}

	void on_hover_begin_hyperlink(builder* b, id widget)
	{
		SFG::window::set_cursor_state(SFG::cursor_state::hand);
	}

	void on_hover_end_hyperlink(builder* b, id widget)
	{
		SFG::window::set_cursor_state(SFG::cursor_state::arrow);
	}

	gui_builder::gui_builder_style::gui_builder_style()
	{
		col_title_line_start = SFG::color::from255(91.0f, 0.0f, 72.0f, 0.0f).srgb_to_linear().to_vector();
		col_title_line_end	 = SFG::color::from255(151.0f, 0.0f, 119.0f, 255.0f).srgb_to_linear().to_vector();
		col_hyperlink		 = SFG::color::from255(7, 131, 214, 255.0f).srgb_to_linear().to_vector();

		col_title	 = SFG::color::from255(180, 180, 180, 255).srgb_to_linear().to_vector();
		col_text	 = SFG::color::from255(180, 180, 180, 255).srgb_to_linear().to_vector();
		col_frame_bg = SFG::color::from255(4, 4, 4, 255).srgb_to_linear().to_vector();
		col_area_bg	 = SFG::color::from255(14, 14, 14, 255).srgb_to_linear().to_vector();
		col_root	 = SFG::color::from255(30, 30, 30, 255).srgb_to_linear().to_vector();

		root_margin		  = 16.0f;
		item_spacing	  = 18.0f;
		title_line_width  = 0.8f;
		title_line_height = 4.0f;
	}

	id gui_builder::begin_root()
	{
		const id w = new_widget(true);

		// gfx
		_builder->widget_get_gfx(w).color = style.col_root;
		_builder->widget_get_gfx(w).flags = gfx_flags::gfx_is_rect;

		// positioning
		_builder->widget_set_pos_abs(w, VEKT_VEC2());
		_builder->widget_set_size_abs(w, VEKT_VEC2(100, 100));

		// sizes
		_builder->widget_get_size_props(w).spacing		 = style.item_spacing;
		_builder->widget_get_size_props(w).child_margins = {style.root_margin, style.root_margin, style.root_margin, style.root_margin};
		pos_props& pp									 = _builder->widget_get_pos_props(w);
		pp.flags										 = pos_flags::pf_child_pos_column;

		return w;
	}

	void gui_builder::end_root()
	{
		pop_stack();
	}

	id gui_builder::begin_area()
	{
		const id w = new_widget(true);

		// gfx
		_builder->widget_get_gfx(w).color = style.col_area_bg;
		_builder->widget_get_gfx(w).flags = gfx_flags::gfx_is_rect;

		// positioning
		_builder->widget_set_pos(w, VEKT_VEC2());
		_builder->widget_set_size(w, SFG::vector2::one);
		_builder->widget_get_size_props(w).spacing		 = style.item_spacing;
		_builder->widget_get_size_props(w).child_margins = {style.root_margin, style.root_margin, style.root_margin, style.root_margin};
		_builder->widget_get_pos_props(w).flags			 = pos_flags::pf_child_pos_column;

		return w;
	}

	void gui_builder::end_area()
	{
		pop_stack();
	}

	id gui_builder::add_title(const char* title)
	{
		const id w = new_widget();
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_child_pos_column | pos_flags::pf_x_relative;
			pp.pos.x	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.spacing	   = style.item_spacing / 2.0f;
			sz.flags	   = size_flags::sf_x_relative | size_flags::sf_y_total_children;
			sz.size.x	   = 1.0f;
		}

		const id line = _builder->allocate();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(line);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_second_color;
			gfx.color		= style.col_title_line_start;

			second_color_props& sc = _builder->widget_get_second_color(line);
			sc.color			   = style.col_title_line_end;

			pos_props& pp = _builder->widget_get_pos_props(line);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_x_anchor_end;
			pp.pos.x	  = 1.0f;

			size_props& sz = _builder->widget_get_size_props(line);
			sz.size		   = VEKT_VEC2(style.title_line_width, style.title_line_height);
			sz.flags	   = size_flags::sf_x_relative | size_flags::sf_y_abs;
		}

		const id txt = _builder->allocate();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags		= gfx_flags::gfx_is_text;
			gfx.color		= style.col_title;

			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_x_anchor_end;
			pp.pos.x	  = 1.0f;

			text_props& tp = _builder->widget_get_text(txt);
			tp.text		   = title;
			tp.font		   = style.active_font;
			_builder->widget_update_text(txt);
		}

		_builder->widget_add_child(w, txt);
		_builder->widget_add_child(w, line);

		return w;
	}

	id gui_builder::add_label(const char* label)
	{
		const id w = new_widget();

		widget_gfx& gfx = _builder->widget_get_gfx(w);
		gfx.flags		= gfx_flags::gfx_is_text;
		gfx.color		= style.col_text;

		pos_props& pp = _builder->widget_get_pos_props(w);
		pp.flags	  = pos_flags::pf_x_relative;
		pp.pos.x	  = 0.0f;

		text_props& tp = _builder->widget_get_text(w);
		tp.text		   = label;
		tp.font		   = style.active_font;
		_builder->widget_update_text(w);
		return w;
	}

	id gui_builder::add_hyperlink(const char* label, const char* link)
	{
		const id w = new_widget();
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_child_pos_column | pos_flags::pf_x_relative;
			pp.pos.x	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.spacing	   = 1.0f;
			sz.flags	   = size_flags::sf_x_max_children | size_flags::sf_y_total_children;
			sz.size.x	   = 1.0f;

			_builder->widget_get_hover_callbacks(w).on_hover_begin = on_hover_begin_hyperlink;
			_builder->widget_get_hover_callbacks(w).on_hover_end   = on_hover_end_hyperlink;
			_builder->widget_get_mouse_callbacks(w).on_mouse	   = on_mouse;
		}

		const id txt = _builder->allocate();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags		= gfx_flags::gfx_is_text;
			gfx.color		= style.col_hyperlink;

			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_x_relative;
			pp.pos.x	  = 0.0f;

			text_props& tp = _builder->widget_get_text(txt);
			tp.text		   = label;
			tp.font		   = style.active_font;
			_builder->widget_update_text(txt);
		}

		const id line = _builder->allocate();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(line);
			gfx.flags		= gfx_flags::gfx_is_rect;
			gfx.color		= style.col_hyperlink;

			pos_props& pp = _builder->widget_get_pos_props(line);
			pp.flags	  = pos_flags::pf_x_relative;
			pp.pos.x	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(line);
			sz.size		   = VEKT_VEC2(.0f, 1.0f);
			sz.flags	   = size_flags::sf_x_fill | size_flags::sf_y_abs;
		}

		_builder->widget_add_child(w, txt);
		_builder->widget_add_child(w, line);

		return w;
	}

	id gui_builder::new_widget(bool push_to_stack)
	{
		const id w = _builder->allocate();

		const id parent = _stack_ptr == 0 ? NULL_WIDGET_ID : stack();

		if (parent != NULL_WIDGET_ID)
			_builder->widget_add_child(parent, w);

		if (push_to_stack)
			push_stack(w);
		return w;
	}

	void gui_builder::push_stack(id s)
	{
		_stack[_stack_ptr] = s;
		_stack_ptr++;
		ASSERT(_stack_ptr < STACK_SIZE);
	}

	id gui_builder::pop_stack()
	{
		ASSERT(_stack_ptr > 0);
		const id last = _stack[_stack_ptr - 1];
		_stack_ptr--;
		return last;
	}

	id gui_builder::stack()
	{
		ASSERT(_stack_ptr > 0);
		return _stack[_stack_ptr - 1];
	}
};
