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

#include "gui_builder.hpp "
#include "io/log.hpp"
#include "io/assert.hpp"
#include "vekt.hpp"
#include "data/string_util.hpp"
#include "data/vector_util.hpp"
#include "math/color.hpp"
#include "math/math.hpp"
#include "memory/text_allocator.hpp"
#include "memory/memory.hpp"
#include "common/system_info.hpp"
#include "input/input_mappings.hpp"

#include "platform/window.hpp"
#include "platform/process.hpp"

#undef min
#undef max

namespace SFG
{
	using namespace vekt;

	float gui_builder::gui_builder_style::DPI_SCALE = 1.0f;

	// -----------------------------------------------------------------------------
	// gui text field
	// -----------------------------------------------------------------------------

	namespace
	{
		static inline unsigned int umin(unsigned int a, unsigned int b)
		{
			return a < b ? a : b;
		}
		static inline unsigned int umax(unsigned int a, unsigned int b)
		{
			return a > b ? a : b;
		}
	}

	unsigned int gui_text_field::selection_min() const
	{
		return umin(caret_pos, caret_end_pos);
	}
	unsigned int gui_text_field::selection_max() const
	{
		return umax(caret_pos, caret_end_pos);
	}
	void gui_text_field::collapse_caret_to(unsigned int p)
	{
		p			  = umin(p, buffer_size);
		caret_pos	  = p;
		caret_end_pos = p;
	}

	void gui_text_field::delete_range(unsigned int from, unsigned int to)
	{
		if (from > to)
		{
			unsigned int tmp = from;
			from			 = to;
			to				 = tmp;
		}
		from = umin(from, buffer_size);
		to	 = umin(to, buffer_size);
		if (from == to)
			return;

		const unsigned int tail_count = (buffer_size - to) + 1;
		SFG_MEMMOVE((char*)buffer + from, buffer + to, tail_count);

		buffer_size -= (to - from);
		collapse_caret_to(from);
	}
	bool gui_text_field::delete_selection_if_any()
	{
		const unsigned int mn = selection_min();
		const unsigned int mx = selection_max();
		if (mn == mx)
			return false;
		delete_range(mn, mx);
		return true;
	}
	void gui_text_field::insert_string_at_caret(const char* s, unsigned int len)
	{
		if (!s || len == 0)
			return;

		delete_selection_if_any();

		const unsigned int available = (buffer_capacity > 0) ? (buffer_capacity - 1) : 0;
		if (buffer_size >= available)
			return;

		const unsigned int can_add = umin(len, available - buffer_size);
		if (can_add == 0)
			return;

		const unsigned int pos = umin(caret_pos, buffer_size);

		const unsigned int tail_count = (buffer_size - pos) + 1;
		SFG_MEMMOVE((char*)buffer + pos + can_add, buffer + pos, tail_count);
		SFG_MEMCPY((char*)buffer + pos, s, can_add);

		buffer_size += can_add;
		collapse_caret_to(pos + can_add);
	}
	void gui_text_field::insert_char_at_caret(char c)
	{
		insert_string_at_caret(&c, 1);
	}

	namespace
	{
		void on_hover_begin_hand_c(vekt::builder* b, vekt::id widget)
		{
			SFG::window::set_cursor_state(SFG::cursor_state::hand);
		}

		void on_hover_end_hand_c(vekt::builder* b, vekt::id widget)
		{
			SFG::window::set_cursor_state(SFG::cursor_state::arrow);
		}

		void on_hover_begin_text_field(vekt::builder* b, vekt::id widget)
		{
			SFG::window::set_cursor_state(SFG::cursor_state::caret);
		}

		void on_hover_end_text_field(vekt::builder* b, vekt::id widget)
		{
			SFG::window::set_cursor_state(SFG::cursor_state::arrow);
		}

	}

	vekt::input_event_result gui_builder::on_text_field_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		const hover_callback& hb = b->widget_get_hover_callbacks(widget);
		if (!hb.is_hovered)
			return vekt::input_event_result::not_handled;

		if (ev.type == vekt::input_event_type::released)
			return vekt::input_event_result::not_handled;

		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it = vector_util::find_if(gb->_text_fields, [widget](const gui_text_field& tf) -> bool { return tf.widget == widget; });
		SFG_ASSERT(it != gb->_text_fields.end());

		if (ev.type == vekt::input_event_type::repeated)
		{
			it->caret_pos	  = 0;
			it->caret_end_pos = it->buffer_size;
			return vekt::input_event_result::handled;
		}

		const vekt::id text_widget = it->text_widget;

		if (it->buffer_size == 0)
		{
			it->caret_end_pos = 0;
		}
		else
		{
			const size_props& sz	 = b->widget_get_size_props(widget);
			const vector2	  pos	 = b->widget_get_pos(widget) + vector2(sz.child_margins.left, 0.0f);
			const float		  x_diff = ev.position.x - pos.x;

			it->caret_pos = math::clamp(b->widget_get_character_index(it->text_widget, x_diff), (uint32)0, it->buffer_size);
		}
		it->caret_end_pos = it->caret_pos;
		return vekt::input_event_result::handled;
	}

	void gui_builder::on_text_field_drag(vekt::builder* b, vekt::id widget, float mp_x, float mp_y, float delta_x, float delta_y, unsigned int button)
	{
		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it = vector_util::find_if(gb->_text_fields, [widget](const gui_text_field& tf) -> bool { return tf.widget == widget; });
		SFG_ASSERT(it != gb->_text_fields.end());

		if (button == input_code::mouse_middle)
		{
			if (math::almost_equal(it->value_increment, 0.0f))
				return;

			it->value += math::clamp(delta_x, -1.0f, 1.0f) * it->value_increment;
			gb->text_field_edit_complete(*it);
			return;
		}

		if (button != input_code::mouse_0)
			return;

		if (it->buffer_size == 0)
		{
			it->caret_end_pos = 0;
			return;
		}
		const vekt::id text_widget = it->text_widget;

		const size_props& sz	 = b->widget_get_size_props(widget);
		const vector2	  pos	 = b->widget_get_pos(widget) + vector2(sz.child_margins.left, 0.0f);
		const float		  x_diff = mp_x - pos.x;

		if (x_diff < 0)
		{
			it->caret_end_pos = 0;
			return;
		}
		it->caret_end_pos = math::clamp(b->widget_get_character_index(it->text_widget, x_diff), (uint32)0, it->buffer_size);
	}

	void gui_builder::on_text_field_focus_lost(vekt::builder* b, vekt::id widget)
	{
		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it = vector_util::find_if(gb->_text_fields, [widget](const gui_text_field& tf) -> bool { return tf.widget == widget; });
		SFG_ASSERT(it != gb->_text_fields.end());
		gb->text_field_edit_complete(*it);
	}

	void gui_builder::on_text_field_focus_gained(vekt::builder* b, vekt::id widget, bool from_nav)
	{

		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it = vector_util::find_if(gb->_text_fields, [widget](const gui_text_field& tf) -> bool { return tf.widget == widget; });
		SFG_ASSERT(it != gb->_text_fields.end());

		if (from_nav)
		{
			it->caret_pos	  = 0;
			it->caret_end_pos = it->buffer_size;
		}
	}

	void gui_builder::on_context_item_hover_begin(vekt::builder* b, vekt::id widget)
	{
		gui_builder* gb					= static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		b->widget_get_gfx(widget).color = gb->style.col_highlight_transparent;
	}

	void gui_builder::on_context_item_hover_end(vekt::builder* b, vekt::id widget)
	{
		b->widget_get_gfx(widget).color = {};
	}

	void gui_builder::on_text_field_draw(vekt::builder* b, vekt::id widget)
	{
		hover_callback& hb = b->widget_get_hover_callbacks(widget);
		if (!hb.is_focused)
			return;

		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it = vector_util::find_if(gb->_text_fields, [widget](const gui_text_field& tf) -> bool { return tf.widget == widget; });
		SFG_ASSERT(it != gb->_text_fields.end());

		const gui_text_field& tf = *it;

		const size_props& sz		= b->widget_get_size_props(widget);
		const vector2	  pos		= b->widget_get_pos(widget) + vector2(sz.child_margins.left, 0.0f);
		const vector2	  text_size = b->widget_get_size(it->text_widget);

		const unsigned int min		  = math::min(tf.caret_pos, tf.caret_end_pos);
		const unsigned int max		  = math::max(tf.caret_pos, tf.caret_end_pos);
		const float		   min_offset = b->widget_get_character_offset(tf.text_widget, min);
		const float		   max_offset = b->widget_get_character_offset(tf.text_widget, max);

		widget_gfx gfx = {
			.flags = gfx_flags::gfx_is_rect,
		};

		static float timer = 0.0f;

		if (timer > 500)
		{
			// caret

			b->add_filled_rect({
				.gfx			 = gfx,
				.min			 = pos + vector2(min_offset, sz.size.y * 0.1f),
				.max			 = pos + vector2(min_offset + 1.0f * gui_builder::gui_builder_style::DPI_SCALE, sz.size.y - (sz.size.y * 0.1f)),
				.color_start	 = gb->style.col_text_dim,
				.color_end		 = gb->style.col_text_dim,
				.color_direction = vekt::direction::horizontal,
				.widget_id		 = widget,
				.multi_color	 = false,
			});

			if (timer > 1500)
				timer = 0.0f;
		}

		timer += frame_info::get_main_thread_time_milli();

		if (min == max)
			return;

		// highlight
		b->add_filled_rect({
			.gfx			 = gfx,
			.min			 = pos + vector2(min_offset, sz.size.y * 0.1f),
			.max			 = pos + vector2(max_offset, sz.size.y - (sz.size.y * 0.1f)),
			.color_start	 = gb->style.col_highlight_transparent,
			.color_end		 = gb->style.col_highlight_transparent,
			.color_direction = vekt::direction::horizontal,
			.widget_id		 = widget,
			.multi_color	 = false,
		});
	}

	vekt::input_event_result gui_builder::on_text_field_key(vekt::builder* b, vekt::id widget, const vekt::key_event& ev)
	{
		if (ev.type != vekt::input_event_type::pressed || (ev.type == vekt::input_event_type::repeated && ev.key == input_code::key_backspace))
			return vekt::input_event_result::not_handled;

		gui_builder* gb = static_cast<gui_builder*>(b->widget_get_user_data(widget).ptr);
		auto		 it = vector_util::find_if(gb->_text_fields, [widget](const gui_text_field& tf) -> bool { return tf.widget == widget; });
		SFG_ASSERT(it != gb->_text_fields.end());

		gui_text_field& tf	   = *it;
		char*			buffer = (char*)tf.buffer;

		const unsigned int capacity = tf.buffer_capacity;
		SFG_ASSERT(capacity != 0);

		// Clamp caret positions into [0, tf.buffer_size]
		tf.caret_pos	 = umin(tf.caret_pos, tf.buffer_size);
		tf.caret_end_pos = umin(tf.caret_end_pos, tf.buffer_size);

		const bool ctrl = window::is_key_down(input_code::key_lctrl);

		bool text_changed = false;

		if (ev.key == input_code::key_left)
		{
			const unsigned int mn = tf.selection_min();
			if (mn > 0)
				tf.collapse_caret_to(mn - 1);
			else
				tf.collapse_caret_to(0);
			return vekt::input_event_result::handled;
		}
		else if (ev.key == input_code::key_right)
		{
			const unsigned int mx = tf.selection_max();
			if (mx < tf.buffer_size)
				tf.collapse_caret_to(mx + 1);
			else
				tf.collapse_caret_to(tf.buffer_size);
			return vekt::input_event_result::handled;
		}
		else if (ev.key == input_code::key_return)
		{
			gb->text_field_edit_complete(tf);
			return vekt::input_event_result::handled;
		}
		else if (ev.key == input_code::key_backspace)
		{
			if (!tf.delete_selection_if_any())
			{
				if (tf.caret_pos > 0)
				{
					const unsigned int p = umin(tf.caret_pos, tf.buffer_size);
					tf.delete_range(p - 1, p);
				}
			}
			text_changed = true;
		}
		else if (ev.key == input_code::key_a && ctrl)
		{
			tf.caret_pos	 = 0;
			tf.caret_end_pos = tf.buffer_size;
			return vekt::input_event_result::handled;
		}
		else if (ev.key == input_code::key_x && ctrl)
		{
			const unsigned int mn = tf.selection_min();
			const unsigned int mx = tf.selection_max();
			if (mn != mx)
			{
				const string slice(buffer + mn, buffer + mx);
				process::push_clipboard(slice.c_str());

				if (!tf.delete_selection_if_any())
				{
					if (tf.caret_pos > 0)
					{
						const unsigned int p = umin(tf.caret_pos, tf.buffer_size);
						tf.delete_range(p - 1, p);
					}
				}
				text_changed = true;
			}
		}
		else if (ev.key == input_code::key_c && ctrl)
		{
			const unsigned int mn = tf.selection_min();
			const unsigned int mx = tf.selection_max();
			if (mn != mx)
			{
				const string slice(buffer + mn, buffer + mx);
				process::push_clipboard(slice.c_str());
			}
			return vekt::input_event_result::handled;
		}
		else if (ev.key == input_code::key_v && ctrl)
		{
			const string clip = process::get_clipboard();
			tf.insert_string_at_caret(clip.c_str(), (unsigned int)clip.size());
			text_changed = true;

			return vekt::input_event_result::handled;
		}
		else
		{
			const char	 c	  = process::get_character_from_key(static_cast<uint32>(ev.key));
			const uint16 mask = process::get_character_mask_from_key(static_cast<uint32>(ev.key), c);
			if (!(mask & character_mask::printable))
				return vekt::input_event_result::not_handled;

			tf.insert_char_at_caret(c);
			text_changed = true;
		}

		if (text_changed)
		{
			if (capacity > 0)
			{
				const unsigned int safe_end = umin(tf.buffer_size, capacity - 1);
				buffer[safe_end]			= '\0';
				tf.buffer_size				= safe_end;
			}

			if (tf.type == gui_text_field_type::number)
			{
				if (tf.decimals == 0)
				{
					int val;
					if (string_util::to_int(tf.buffer, val))
						tf.value = val;
					else
						tf.value = 0.0f;
				}
				else
				{
					float  val	 = 0.0f;
					uint32 out_d = 0;
					if (string_util::to_float(tf.buffer, val, out_d))
					{
						tf.value = val;
					}
					else
						tf.value = 0.0f;
				}
			}

			if (gb->callbacks.on_input_field_changed)
				gb->callbacks.on_input_field_changed(widget, tf.buffer, tf.value);
			b->widget_update_text(tf.text_widget);
		}

		return vekt::input_event_result::handled;
	}
	void gui_builder::gui_builder_style::init_defaults()
	{
		col_accent			  = color::from255(151.0f, 0.0f, 119.0f, 255.0f).srgb_to_linear().to_vector();
		col_accent_second	  = color::from255(7, 131, 214, 255.0f).srgb_to_linear().to_vector();
		col_accent_second_dim = color::from255(7, 131, 214, 150.0f).srgb_to_linear().to_vector();
		col_title_line_start  = color::from255(91.0f, 0.0f, 72.0f, 0.0f).srgb_to_linear().to_vector();
		col_title_line_end	  = color::from255(151.0f, 0.0f, 119.0f, 255.0f).srgb_to_linear().to_vector();
		col_hyperlink		  = color::from255(7, 131, 214, 255.0f).srgb_to_linear().to_vector();

		col_highlight				= col_accent_second;
		col_highlight_transparent	= col_accent_second;
		col_highlight_transparent.w = 0.5f;

		col_title	 = color::from255(180, 180, 180, 255).srgb_to_linear().to_vector();
		col_text	 = color::from255(180, 180, 180, 255).srgb_to_linear().to_vector();
		col_text_dim = color::from255(130, 130, 130, 255).srgb_to_linear().to_vector();
		col_frame_bg = color::from255(4, 4, 4, 255).srgb_to_linear().to_vector();
		col_area_bg	 = color::from255(15, 15, 15, 255).srgb_to_linear().to_vector();
		col_root	 = color::from255(28, 28, 28, 255).srgb_to_linear().to_vector();

		col_scroll_bar			 = col_accent;
		col_scroll_bar_bg		 = col_frame_bg;
		col_button				 = col_root;
		col_button_hover		 = col_area_bg;
		col_button_press		 = col_frame_bg;
		col_frame_outline		 = color::from255(60, 60, 60, 255).srgb_to_linear().to_vector();
		col_context_menu_outline = color::from255(60, 60, 60, 255).srgb_to_linear().to_vector();

		root_rounding = 6.0f;

		outer_margin	  = DPI_SCALE * 8;
		item_spacing	  = DPI_SCALE * 3;
		root_spacing	  = DPI_SCALE * 6;
		row_spacing		  = DPI_SCALE * 6;
		row_height		  = DPI_SCALE * 20;
		title_line_width  = 0.8f;
		title_line_height = DPI_SCALE * 2;

		item_height			= DPI_SCALE * 16;
		table_cell_height	= DPI_SCALE * 10;
		seperator_thickness = DPI_SCALE * 1;
		property_cell_div	= 0.3f;

		area_rounding	 = 8.0f;
		scroll_thickness = DPI_SCALE * 4;
		scroll_rounding	 = 8.0f;

		inner_margin				   = DPI_SCALE * 4;
		frame_thickness				   = DPI_SCALE * 1;
		frame_rounding				   = 2.0f;
		context_menu_outline_thickness = DPI_SCALE * 1.2f;
	}

	// -----------------------------------------------------------------------------
	// big layout
	// -----------------------------------------------------------------------------

	void gui_builder::init(vekt::builder* b, text_allocator* alloc)
	{
		_builder   = b;
		_txt_alloc = alloc;
		_text_fields.reserve(256);

		const id w = _builder->allocate();

		// gfx
		_builder->widget_get_gfx(w).color = style.col_root;
		_builder->widget_get_gfx(w).flags = gfx_flags::gfx_is_rect | gfx_flags::gfx_has_rounding;
		rounding_props& rp				  = _builder->widget_get_rounding(w);
		rp.segments						  = 16;
		rp.rounding						  = style.root_rounding;

		// positioning
		_builder->widget_set_pos_abs(w, VEKT_VEC2());
		_builder->widget_set_size_abs(w, VEKT_VEC2(100, 100));

		// sizes
		_builder->widget_get_size_props(w).spacing		 = style.root_spacing;
		_builder->widget_get_size_props(w).child_margins = {style.outer_margin, style.outer_margin, style.outer_margin, style.outer_margin};
		pos_props& pp									 = _builder->widget_get_pos_props(w);
		pp.flags										 = pos_flags::pf_child_pos_column;
		_root											 = w;
	}

	void gui_builder::uninit()
	{
		for (const gui_text_field& tf : _text_fields)
		{
			_txt_alloc->deallocate(tf.buffer);
		}
		_text_fields.resize(0);
		_builder->deallocate(_root);
		_root = NULL_WIDGET_ID;
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
			sz.child_margins = {style.outer_margin, style.outer_margin, style.outer_margin, style.outer_margin};

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
			sz.flags	   = size_flags::sf_x_abs | size_flags::sf_y_relative;
			sz.size.x	   = style.scroll_thickness;
			sz.size.y	   = 1.0f;

			widget_gfx& gfx = _builder->widget_get_gfx(scroll_bg);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_rounding;
			gfx.color		= style.col_scroll_bar_bg;

			rounding_props& rp = _builder->widget_get_rounding(scroll_bg);
			rp.segments		   = 16;
			rp.rounding		   = style.scroll_rounding;
		}

		const id scroll = new_widget();
		{
			pos_props& pp = _builder->widget_get_pos_props(scroll);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative;
			pp.pos		  = VEKT_VEC2(0.0f, 0.0f);

			size_props& sz = _builder->widget_get_size_props(scroll);
			sz.flags	   = size_flags::sf_x_relative;
			sz.size.x	   = 1.0f;

			scroll_props& sc = _builder->widget_get_scroll_props(scroll);
			sc.scroll_parent = w;

			widget_gfx& gfx = _builder->widget_get_gfx(scroll);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_rounding;
			gfx.color		= style.col_scroll_bar;

			rounding_props& rp = _builder->widget_get_rounding(scroll);
			rp.segments		   = 16;
			rp.rounding		   = style.scroll_rounding;

			hover_callback& hb = _builder->widget_get_hover_callbacks(scroll);
			hb.on_hover_begin  = on_hover_begin_hand_c;
			hb.on_hover_end	   = on_hover_end_hand_c;
			hb.receive_mouse   = 1;
		}

		pop_stack();

		return w;
	}

	void gui_builder::end_area()
	{
		pop_stack();
	}

	vekt::id gui_builder::begin_context_menu(float abs_x, float abs_y)
	{
		const id w = _builder->allocate();
		push_stack(w);
		_builder->widget_add_child(_builder->get_root(), w);
		{
			pos_props& p = _builder->widget_get_pos_props(w);
			p.flags		 = pos_flags::pf_x_abs | pos_flags::pf_y_abs | pos_flags::pf_child_pos_column;
			p.pos.x		 = abs_x;
			p.pos.y		 = abs_y;

			size_props& sz	 = _builder->widget_get_size_props(w);
			sz.flags		 = size_flags::sf_x_max_children | size_flags::sf_y_total_children;
			sz.child_margins = {style.inner_margin, style.inner_margin, 0.0f, 0.0f};
			sz.spacing		 = style.item_spacing;

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_stroke;
			gfx.color		= style.col_area_bg;
			gfx.draw_order	= 1;

			stroke_props& sp = _builder->widget_get_stroke(w);
			sp.thickness	 = style.context_menu_outline_thickness;
			sp.color		 = style.col_context_menu_outline;
		}

		return w;
	}

	vekt::id gui_builder::add_context_menu_item(const char* label)
	{
		const id w = new_widget(true);
		{
			pos_props& p = _builder->widget_get_pos_props(w);
			p.flags		 = pos_flags::pf_x_relative | pos_flags::pf_child_pos_row;
			p.pos.x		 = 0.0f;

			size_props& sz	 = _builder->widget_get_size_props(w);
			sz.flags		 = size_flags::sf_x_abs | size_flags::sf_y_abs;
			sz.size.y		 = style.item_height;
			sz.size.x		 = 200;
			sz.child_margins = {0.0f, 0.0f, style.inner_margin, style.inner_margin};
			sz.spacing		 = style.row_spacing;

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect;
			gfx.color		= {0.0f, 0.0f, 0.0f, 0.0f};
			gfx.draw_order	= 2;

			hover_callback& hb = _builder->widget_get_hover_callbacks(w);
			hb.on_hover_begin  = on_context_item_hover_begin;
			hb.on_hover_end	   = on_context_item_hover_end;

			mouse_callback& mb = _builder->widget_get_mouse_callbacks(w);
			mb.on_mouse		   = callbacks.on_mouse;

			widget_user_data& ud = _builder->widget_get_user_data(w);
			ud.ptr				 = this;
		}

		const id txt = new_widget();
		{
			widget_gfx& gfx = _builder->widget_get_gfx(txt);
			gfx.flags		= gfx_flags::gfx_is_text;
			gfx.color		= style.col_text;
			gfx.draw_order	= 3;

			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
			pp.pos.y	  = 0.5f;

			text_props& tp = _builder->widget_get_text(txt);
			tp.text		   = _txt_alloc->allocate(label);
			tp.font		   = style.default_font;
			_builder->widget_update_text(txt);
		}

		pop_stack();
		return w;
	}

	void gui_builder::end_context_menu()
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

	gui_builder::id_pair gui_builder::add_property_single_button(const char* label)
	{
		const id	  row = add_property_row();
		const id_pair p	  = add_button(label);
		pop_stack();
		return p;
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

	gui_builder::id_pair gui_builder::add_property_row_text_field(const char* label, const char* text, unsigned int max_text_size, gui_text_field_type type, unsigned int decimals, float increment)
	{
		const id row = add_property_row();

		add_row_cell(style.property_cell_div);
		add_label(label);
		pop_stack();

		add_row_cell_seperator();

		add_row_cell(0.0f);
		const id_pair field = add_text_field(text, max_text_size, type, decimals, increment);
		pop_stack();
		pop_stack();
		return field;
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

			size_props& sz		   = _builder->widget_get_size_props(w);
			sz.flags			   = size_flags::sf_x_relative | size_flags::sf_y_abs;
			sz.size.x			   = 1.0f;
			sz.size.y			   = style.row_height;
			sz.spacing			   = style.row_spacing;
			sz.child_margins.right = style.outer_margin;

			// widget_gfx& gfx = _builder->widget_get_gfx(w);
			// gfx.flags		= gfx_flags::gfx_is_rect;
			// gfx.color		= style.col_accent;
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
			sz.size.y	   = 1.0f;

			if (vekt::math::equals(size, 0.0f))
				sz.flags |= size_flags::sf_x_fill;
			else
			{
				sz.flags |= size_flags::sf_x_relative;
				sz.size.x = size;
			}

			// widget_gfx& gfx = _builder->widget_get_gfx(w);
			// gfx.flags		= gfx_flags::gfx_is_rect;
			// gfx.color		= style.col_accent;
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
			tp.text		   = _txt_alloc->allocate(title);
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
		tp.text		   = _txt_alloc->allocate(label);
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

			_builder->widget_get_hover_callbacks(w).on_hover_begin = on_hover_begin_hand_c;
			_builder->widget_get_hover_callbacks(w).on_hover_end   = on_hover_end_hand_c;
			_builder->widget_get_hover_callbacks(w).receive_mouse  = 1;
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
			tp.text		   = _txt_alloc->allocate(label);
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

	gui_builder::id_pair gui_builder::add_button(const char* title)
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
			pp.pos.y	  = 0.5f;

			size_props& sz	 = _builder->widget_get_size_props(w);
			sz.flags		 = size_flags::sf_x_max_children | size_flags::sf_y_abs;
			sz.size.y		 = style.item_height;
			sz.child_margins = {style.inner_margin, style.inner_margin, style.inner_margin, style.inner_margin};

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_stroke | gfx_flags::gfx_has_rounding | gfx_flags::gfx_has_press_color | gfx_flags::gfx_has_hover_color;
			gfx.color		= style.col_button;

			stroke_props& st = _builder->widget_get_stroke(w);
			st.thickness	 = style.frame_thickness;
			st.color		 = style.col_frame_outline;

			rounding_props& rp = _builder->widget_get_rounding(w);
			rp.rounding		   = style.frame_rounding;
			rp.segments		   = 8;

			input_color_props& icp = _builder->widget_get_input_colors(w);
			icp.pressed_color	   = style.col_accent_second_dim;
			icp.hovered_color	   = style.col_accent_second;

			mouse_callback& mc = _builder->widget_get_mouse_callbacks(w);
			mc.on_mouse		   = callbacks.on_mouse;

			_builder->widget_get_hover_callbacks(w).receive_mouse = 1;
		}

		const id txt = add_label(title);
		{
			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_x_anchor_center | pos_flags::pf_y_anchor_center;
			pp.pos.x	  = 0.5f;
			pp.pos.y	  = 0.5f;
		}

		pop_stack();
		return {w, txt};
	}

	id gui_builder::set_fill_x(vekt::id id)
	{
		size_props& sz = _builder->widget_get_size_props(id);
		sz.flags &= ~size_flags::sf_x_abs;
		sz.flags &= ~size_flags::sf_x_relative;
		sz.flags &= ~size_flags::sf_x_copy_y;
		sz.flags &= ~size_flags::sf_x_max_children;
		sz.flags &= ~size_flags::sf_x_total_children;
		sz.flags |= size_flags::sf_x_fill;
		return id;
	}

	void gui_builder::set_text_field_text(gui_text_field& tf, const char* text)
	{
		const size_t sz = strlen(text);
		SFG_ASSERT(sz < tf.buffer_capacity);
		char* c = (char*)tf.buffer;

		if (sz == 0)
		{
			c[0]			 = '\0';
			tf.buffer_size	 = 0;
			tf.caret_end_pos = tf.caret_pos = 0;
			return;
		}

		SFG_MEMCPY(c, (char*)text, sz);
		tf.buffer_size	  = sz;
		c[tf.buffer_size] = '\0';
		tf.caret_pos	  = math::clamp(tf.caret_pos, (uint32)0, tf.buffer_size);
		tf.caret_end_pos  = tf.caret_pos;
		_builder->widget_update_text(tf.text_widget);
	}

	void gui_builder::set_text_field_text(vekt::id id, const char* text)
	{
		auto it = vector_util::find_if(_text_fields, [id](const gui_text_field& tf) -> bool { return tf.widget == id; });
		SFG_ASSERT(it != _text_fields.end());
		set_text_field_text(*it, text);
	}

	void gui_builder::text_field_edit_complete(gui_text_field& tf)
	{
		if (tf.type == gui_text_field_type::number)
		{
			if (tf.decimals == 0)
			{
				const int val = static_cast<int>(tf.value);
				set_text_field_text(tf, std::to_string(val).c_str());
			}
			else
			{
				int written = string_util::append_float(tf.value, (char*)tf.buffer, 16, tf.decimals, true);
				_builder->widget_update_text(tf.text_widget);
				tf.buffer_size = tf.caret_pos = tf.caret_end_pos = written;
			}
		}
	}

	gui_builder::id_pair gui_builder::add_text_field(const char* text, unsigned int max_size, gui_text_field_type type, unsigned int decimals, float increment)
	{
		const id w = new_widget(true);
		{
			pos_props& pp = _builder->widget_get_pos_props(w);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
			pp.pos.y	  = 0.5f;
			pp.pos.x	  = 0.0f;

			size_props& sz	 = _builder->widget_get_size_props(w);
			sz.flags		 = size_flags::sf_x_relative | size_flags::sf_y_abs;
			sz.size.x		 = 1.0f;
			sz.size.y		 = style.item_height;
			sz.child_margins = {style.inner_margin, style.inner_margin, style.inner_margin, style.inner_margin};

			widget_gfx& gfx = _builder->widget_get_gfx(w);
			gfx.flags		= gfx_flags::gfx_is_rect | gfx_flags::gfx_has_stroke | gfx_flags::gfx_has_rounding | gfx_flags::gfx_custom_pass | gfx_flags::gfx_focusable;
			gfx.color		= style.col_frame_bg;

			stroke_props& st = _builder->widget_get_stroke(w);
			st.thickness	 = style.frame_thickness;
			st.color		 = style.col_frame_outline;

			rounding_props& rp = _builder->widget_get_rounding(w);
			rp.rounding		   = style.frame_rounding;
			rp.segments		   = 8;

			input_color_props& icp = _builder->widget_get_input_colors(w);
			icp.hovered_color	   = style.col_area_bg;
			icp.focus_color		   = style.col_accent;

			mouse_callback& mc = _builder->widget_get_mouse_callbacks(w);
			mc.on_mouse		   = on_text_field_mouse;
			mc.on_drag		   = on_text_field_drag;

			key_callback& kc = _builder->widget_get_key_callbacks(w);
			kc.on_key		 = on_text_field_key;

			hover_callback& hb = _builder->widget_get_hover_callbacks(w);
			hb.receive_mouse   = 1;
			hb.on_hover_begin  = on_hover_begin_text_field;
			hb.on_hover_end	   = on_hover_end_text_field;
			hb.on_focus_lost   = on_text_field_focus_lost;
			hb.on_focus_gained = on_text_field_focus_gained;

			widget_user_data& ud = _builder->widget_get_user_data(w);
			ud.ptr				 = this;

			custom_passes& cp	= _builder->widget_get_custom_pass(w);
			cp.custom_draw_pass = on_text_field_draw;
		}

		const id txt = add_label(nullptr);
		{
			pos_props& pp = _builder->widget_get_pos_props(txt);
			pp.flags	  = pos_flags::pf_x_relative | pos_flags::pf_y_relative | pos_flags::pf_y_anchor_center;
			pp.pos.x	  = 0.0f;
			pp.pos.y	  = 0.5f;

			ASSERT(max_size != 0);

			const gui_text_field tf = {
				.buffer			 = _txt_alloc->allocate(max_size),
				.widget			 = w,
				.text_widget	 = txt,
				.buffer_size	 = static_cast<uint32>(strlen(text)),
				.buffer_capacity = max_size,
				.decimals		 = decimals,
				.value_increment = increment,
				.type			 = type,
			};

			SFG_MEMCPY((void*)tf.buffer, (void*)text, strlen(text));
			_text_fields.push_back(tf);

			text_props& tp = _builder->widget_get_text(txt);
			tp.text		   = tf.buffer;
			_builder->widget_update_text(txt);
		}

		pop_stack();
		return {w, txt};
	}

	id gui_builder::new_widget(bool push_to_stack)
	{
		const id w = _builder->allocate();

		const id parent = _stack_ptr == 0 ? _root : stack();

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
