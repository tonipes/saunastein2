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

#pragma once

#include "gui/vekt_defines.hpp"
#include "math/vector4.hpp"
#include "data/vector.hpp"

namespace vekt
{
	struct font;
}

namespace SFG
{
	class builder;
	class font;
	class text_allocator;

	struct gui_text_field
	{
		const char*	 buffer		 = nullptr;
		vekt::id	 widget		 = 0;
		vekt::id	 text_widget = 0;
		unsigned int caret_pos;
	};

	class gui_builder
	{
	public:
		struct gui_builder_style
		{
			static float DPI_SCALE;

			void init_defaults();

			vector4 col_title_line_start;
			vector4 col_title_line_end;
			vector4 col_hyperlink;
			vector4 col_accent;
			vector4 col_accent_second;
			vector4 col_accent_second_dim;

			vector4		col_scroll_bar;
			vector4		col_scroll_bar_bg;
			vector4		col_title;
			vector4		col_text;
			vector4		col_frame_bg;
			vector4		col_area_bg;
			vector4		col_root;
			vector4		col_button;
			vector4		col_button_hover;
			vector4		col_button_press;
			vector4		col_frame_outline;
			vekt::font* default_font = nullptr;
			vekt::font* title_font	 = nullptr;

			float root_rounding;

			float outer_margin;
			float item_spacing;
			float row_spacing;
			float title_line_width;
			float title_line_height;
			float item_height;
			float row_height;
			float table_cell_height;
			float property_cell_div;
			float seperator_thickness;
			float area_rounding;
			float scroll_thickness;
			float scroll_rounding;
			float inner_margin;
			float frame_thickness;
			float frame_rounding;
		};

		struct gui_builder_callbacks
		{
			void*			 user_data = nullptr;
			vekt::mouse_func on_mouse  = nullptr;
			vekt::key_func	 on_key	   = nullptr;
		};

		struct id_pair
		{
			vekt::id first;
			vekt::id second;
		};

		gui_builder_style	  style		= {};
		gui_builder_callbacks callbacks = {};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(vekt::builder* b, text_allocator* alloc);
		void uninit();

		// -----------------------------------------------------------------------------
		// big layout
		// -----------------------------------------------------------------------------

		vekt::id begin_area(bool fill = true);
		void	 end_area();

		// -----------------------------------------------------------------------------
		// properties
		// -----------------------------------------------------------------------------

		id_pair	 add_property_row_label(const char* label, const char* label2);
		vekt::id add_property_single_label(const char* label);
		id_pair	 add_property_single_button(const char* label);
		vekt::id add_property_single_hyperlink(const char* label);

		// property row variants for fields
		id_pair	 add_property_row_text_field(const char* label, const char* text);
		vekt::id add_property_row();
		vekt::id add_row_cell(float size);
		vekt::id add_row_cell_seperator();

		// -----------------------------------------------------------------------------
		// raw items
		// -----------------------------------------------------------------------------

		vekt::id add_title(const char* title);
		vekt::id add_label(const char* label);
		vekt::id add_hyperlink(const char* label);
		id_pair	 add_button(const char* title);
		void	 set_fill_x(vekt::id id);

		// raw field widgets
		id_pair add_text_field(const char* text, unsigned int max_size);

		inline void push_title_font(vekt::font* f)
		{
			style.title_font = f;
		}

		inline void push_text_font(vekt::font* f)
		{
			style.default_font = f;
		}

		inline vekt::id get_root() const
		{
			return _root;
		}

		static vekt::input_event_result on_text_field_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase);
		static vekt::input_event_result on_text_field_key(vekt::builder* b, vekt::id widget, const vekt::key_event& ev);

		vekt::id new_widget(bool push_to_stack = false);
		vekt::id pop_stack();
		vekt::id stack();
		void	 push_stack(vekt::id s);

	private:
		static constexpr unsigned int STACK_SIZE = 512;

		text_allocator*		   _txt_alloc		  = nullptr;
		vekt::builder*		   _builder			  = nullptr;
		vector<gui_text_field> _text_fields		  = {};
		vekt::id			   _stack[STACK_SIZE] = {NULL_WIDGET_ID};
		vekt::id			   _root			  = NULL_WIDGET_ID;
		vekt::id			   _stack_ptr		  = 0;
	};

}
