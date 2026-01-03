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
	float gui_builder::gui_builder_style::DPI_SCALE = 1.0f;

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

		col_accent			 = SFG::color::from255(151.0f, 0.0f, 119.0f, 255.0f).srgb_to_linear().to_vector();
		col_accent_second	 = SFG::color::from255(7, 131, 214, 255.0f).srgb_to_linear().to_vector();
		col_title_line_start = SFG::color::from255(91.0f, 0.0f, 72.0f, 0.0f).srgb_to_linear().to_vector();
		col_title_line_end	 = SFG::color::from255(151.0f, 0.0f, 119.0f, 255.0f).srgb_to_linear().to_vector();
		col_hyperlink		 = SFG::color::from255(7, 131, 214, 255.0f).srgb_to_linear().to_vector();

		col_title	 = SFG::color::from255(180, 180, 180, 255).srgb_to_linear().to_vector();
		col_text	 = SFG::color::from255(180, 180, 180, 255).srgb_to_linear().to_vector();
		col_frame_bg = SFG::color::from255(4, 4, 4, 255).srgb_to_linear().to_vector();
		col_area_bg	 = SFG::color::from255(15, 15, 15, 255).srgb_to_linear().to_vector();
		col_root	 = SFG::color::from255(28, 28, 28, 255).srgb_to_linear().to_vector();

		col_scroll_bar	  = col_accent;
		col_scroll_bar_bg = col_frame_bg;

		root_margin		  = DPI_SCALE * 8;
		item_spacing	  = DPI_SCALE * 4;
		title_line_width  = 0.8f;
		title_line_height = DPI_SCALE * 2;

		item_height			= DPI_SCALE * 16;
		table_cell_height	= DPI_SCALE * 10;
		seperator_thickness = DPI_SCALE * 1;
		property_cell_div	= 0.3f;

		area_rounding	 = 8.0f;
		scroll_thickness = DPI_SCALE * 4;
		scroll_rounding	 = 8.0f;
	}

	// -----------------------------------------------------------------------------
	// big layout
	// -----------------------------------------------------------------------------

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

	id gui_builder::begin_area(bool fill)
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_child_pos_column;
			pp.pos.x	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.flags	   = size_flags::sf_x_relative;
			sz.size.x	   = 1.0f;

			if (fill)
			{
				sz.flags |= size_flags::sf_y_fill;
			}
			else
			{
				sz.flags |= size_flags::sf_y_total_children;
			}
			sz.spacing		 = style.item_spacing;
			sz.child_margins = {style.item_spacing, style.item_spacing, style.item_spacing, style.item_spacing};

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_rounding | gfx_flags::gfx_clip_children;
			gfx.color		= style.col_area_bg;

			rounding_props& rp = _builder->widget_get_rounding(w);
			rp.rounding		   = style.area_rounding;
			rp.segments		   = 16;
		}

		const id scroll_bg = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(scroll_bg);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_x_anchor_end | pos_flags::pf_overlay;
			pp.pos		  = VEKT_VEC2(1.0f, 0.0f);

			size_props& sz = _builder->widget_get_size_props(scroll_bg);
			sz.flags	   = size_flags::sf_x_abs | size_flags::sf_y_abs;
			sz.size.x	   = style.scroll_thickness;
			sz.size.y	   = 15.0f;

			widget_gfx& gfx = _builder->widget_get_gfx(scroll_bg);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_rounding;
			gfx.color		= style.col_scroll_bar;

			rounding_props& rp = _builder->widget_get_rounding(scroll_bg);
			rp.segments		   = 16;
			rp.rounding		   = style.scroll_rounding;
		}

		// const id scroll = new_widget();
		// {
		// 	pos_props& pp = _builder->widget_get_pos_props(scroll);
		// 	pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative;
		// 	pp.pos		  = VEKT_VEC2(0.0f, 0.0f);
		//
		// 	size_props& sz = _builder->widget_get_size_props(scroll);
		// 	sz.flags	   = size_flags::sf_x_relative | size_flags::sf_y_abs;
		// 	sz.size.x	   = 1.0f;
		// 	sz.size.y	   = 50;
		//
		// 	widget_gfx& gfx = _builder->widget_get_gfx(scroll);
		// 	gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_rounding;
		// 	gfx.color		= style.col_scroll_bar;
		//
		// 	rounding_props& rp = _builder->widget_get_rounding(scroll);
		// 	rp.segments		   = 16;
		// 	rp.rounding		   = style.scroll_rounding;
		// }

		pop_stack();

		return w;
	}

	void gui_builder::end_area()
	{
		pop_stack();
	}

	// -----------------------------------------------------------------------------
	// property - single
	// -----------------------------------------------------------------------------

	id gui_builder::add_property_single_label(const char* label)
	{
		const id row = add_property_row();
		const id w	 = add_label(label);
		pop_stack();
		return w;
	}

	id gui_builder::add_property_single_hyperlink(const char* label)
	{
		const id row = add_property_row();
		const id w	 = add_hyperlink(label);
		pop_stack();
		return w;
	}

	// -----------------------------------------------------------------------------
	// property - rows
	// -----------------------------------------------------------------------------

	gui_builder::id_pair gui_builder::add_property_row_label(const char* label, const char* text)
	{
		const id row = add_property_row();

		add_row_cell(style.property_cell_div);
		const id id0 = add_label(label);
		pop_stack();

		add_row_cell_seperator();

		add_row_cell(0.0f);
		const id id1 = add_label(text);
		pop_stack();

		pop_stack();

		return {id0, id1};
	}

	// -----------------------------------------------------------------------------
	// property - utils
	// -----------------------------------------------------------------------------

	id gui_builder::add_row_cell_seperator()
	{
		const id w = new_widget();
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_y_relative;
			pp.pos.y	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.flags	   = size_flags::sf_x_abs | size_flags::sf_y_relative;
			sz.size.y	   = 1.0f;
			sz.size.x	   = style.seperator_thickness;

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect;
			gfx.color		= style.col_accent;
		}

		return w;
	}

	id gui_builder::add_property_row()
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_child_pos_row;
			pp.pos.x	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.flags	   = size_flags::sf_x_relative | size_flags::sf_y_abs;
			sz.size.x	   = 1.0f;
			sz.size.y	   = style.item_height;
			sz.spacing	   = style.item_spacing;
		};
		return w;
	}

	id gui_builder::add_row_cell(float size)
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_y_relative;
			pp.pos.y	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.flags	   = size_flags::sf_y_relative;
			if (math::equals(size, 0.0f))
				sz.flags |= size_flags::sf_x_fill;
			else
			{
				sz.flags |= size_flags::sf_x_relative;
				sz.size.x = size;
			}
			sz.size.y = 1.0f;
		}
		return w;
	}

	// -----------------------------------------------------------------------------
	// single items
	// -----------------------------------------------------------------------------

	id gui_builder::add_title(const char* title)
	{
		const id w = new_widget();
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_child_pos_column | pos_flags::pf_x_relative;
			pp.pos.x	  = 0.0f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.spacing	   = style.item_spacing * 0.75f;
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
			tp.font		   = style.title_font;
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
		pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
		pp.pos.x	  = 0.0f;
		pp.pos.y	  = 0.5f;

		text_props& tp = _builder->widget_get_text(w);
		tp.text		   = label;
		tp.font		   = style.default_font;
		_builder->widget_update_text(w);

		return w;
	}

	id gui_builder::add_hyperlink(const char* label)
	{
		const id w = new_widget();
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_child_pos_column | pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
			pp.pos.x	  = 0.0f;
			pp.pos.y	  = 0.5f;

			size_props& sz = _builder->widget_get_size_props(w);
			sz.spacing	   = 1.0f;
			sz.flags	   = size_flags::sf_x_max_children | size_flags::sf_y_total_children;
			sz.size.x	   = 1.0f;

			_builder->widget_get_hover_callbacks(w).on_hover_begin = on_hover_begin_hyperlink;
			_builder->widget_get_hover_callbacks(w).on_hover_end   = on_hover_end_hyperlink;
			_builder->widget_get_mouse_callbacks(w).on_mouse	   = callbacks.on_mouse;
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
			tp.font		   = style.default_font;
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

		_builder->widget_get_user_data(w).ptr = callbacks.user_data;
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
