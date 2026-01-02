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
namespace vekt
{
	gui_builder::gui_builder_style::gui_builder_style()
	{
		title_color		 = SFG::color::from255(220, 220, 220, 255).srgb_to_linear().to_vector();
		text_color		 = SFG::color::from255(220, 220, 220, 255).srgb_to_linear().to_vector();
		frame_background = SFG::color::from255(4, 4, 4, 255).srgb_to_linear().to_vector();
		area_background	 = SFG::color::from255(14, 14, 14, 255).srgb_to_linear().to_vector();
		root_background	 = SFG::color::from255(30, 30, 30, 255).srgb_to_linear().to_vector();
	}

	id gui_builder::begin_root()
	{
		const id w = new_widget(true);

		// gfx
		_builder->widget_get_gfx(w).color = style.root_background;
		_builder->widget_get_gfx(w).flags = gfx_flags::gfx_is_rect;

		// positioning
		_builder->widget_set_pos_abs(w, VEKT_VEC2());
		_builder->widget_set_size_abs(w, VEKT_VEC2(100, 100));

		// sizes
		_builder->widget_get_size_props(w).spacing		 = style.item_spacing;
		_builder->widget_get_size_props(w).child_margins = {style.root_margin, style.root_margin, style.root_margin, style.root_margin};
		pos_props& pp = _builder->widget_get_pos_props(w);
		pp.flags = pos_flags::pf_child_pos_column;

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
		_builder->widget_get_gfx(w).color = style.area_background;
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

		widget_gfx& gfx = _builder->widget_get_gfx(w);
		gfx.flags		= gfx_flags::gfx_is_text;
		gfx.color		= style.title_color;

		pos_props& p = _builder->widget_get_pos_props(w);
		p.flags		 = pos_flags::pf_x_relative | pos_flags::pf_x_anchor_end;
		p.pos.x		 = 1.0f;

		text_props& tp = _builder->widget_get_text(w);
		tp.text		   = title;
		tp.font		   = style.active_font;
		_builder->widget_update_text(w);

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
