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

	enum class gui_text_field_type : unsigned char
	{
		text_only,
		number,
	};

	struct gui_text_field
	{
		const char*			buffer			= nullptr;
		vekt::id			widget			= 0;
		vekt::id			text_widget		= 0;
		unsigned int		caret_pos		= 0;
		unsigned int		buffer_size		= 0;
		unsigned int		caret_end_pos	= 0;
		unsigned int		buffer_capacity = 0;
		unsigned int		decimals		= 0;
		float				value			= 0.0f;
		float				value_increment = 0.0f;
		gui_text_field_type type			= gui_text_field_type::text_only;
		unsigned char		is_editing		= 0;

		unsigned int selection_min() const;
		unsigned int selection_max() const;
		void		 collapse_caret_to(unsigned int p);
		void		 delete_range(unsigned int from, unsigned int to);
		bool		 delete_selection_if_any();
		void		 insert_string_at_caret(const char* s, unsigned int len);
		void		 insert_char_at_caret(char c);
	};

	class gui_builder
	{
	public:
		typedef void (*input_field_fn)(void* callback_ud, vekt::builder* b, vekt::id id, const char* txt, float value);

		struct gui_builder_callbacks
		{
			void*			 user_data				= nullptr;
			void*			 callback_ud			= nullptr;
			vekt::mouse_func on_mouse				= nullptr;
			vekt::key_func	 on_key					= nullptr;
			input_field_fn	 on_input_field_changed = nullptr;
		};

		struct id_pair
		{
			vekt::id first;
			vekt::id second;
		};

		struct id_trip
		{
			vekt::id first;
			vekt::id second;
			vekt::id third;
		};

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

		vekt::id begin_context_menu(float abs_x, float abs_y);
		vekt::id add_context_menu_item(const char* label);
		void	 end_context_menu();

		// -----------------------------------------------------------------------------
		// properties
		// -----------------------------------------------------------------------------

		id_pair	 add_property_row_label(const char* label, const char* label2);
		vekt::id add_property_single_label(const char* label);
		id_pair	 add_property_single_button(const char* label);
		vekt::id add_property_single_hyperlink(const char* label);

		// property row variants for fields
		id_pair	 add_property_row_text_field(const char* label, const char* text, unsigned int max_text_size, gui_text_field_type type = gui_text_field_type::text_only, unsigned int decimals = 0, float increment = 0.0f);
		id_trip	 add_property_row_vector3(const char* label, const char* text, unsigned int max_text_size, unsigned int decimals, float increment);
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
		id_pair	 add_text_field(const char* text, unsigned int max_size, gui_text_field_type type = gui_text_field_type::text_only, unsigned int decimals = 0, float increment = 0.0f);

		// -----------------------------------------------------------------------------
		// util
		// -----------------------------------------------------------------------------

		vekt::id set_fill_x(vekt::id id);
		void	 set_text_field_text(gui_text_field& tf, const char* text);
		void	 set_text_field_text(vekt::id id, const char* text);
		void	 set_text_field_text(vekt::id id, float f, bool skip_if_focused);
		void	 text_field_edit_complete(gui_text_field& tf);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline vekt::id get_root() const
		{
			return _root;
		}

		static vekt::input_event_result on_text_field_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase);
		static vekt::input_event_result on_text_field_key(vekt::builder* b, vekt::id widget, const vekt::key_event& ev);
		static void						on_text_field_draw(vekt::builder* b, vekt::id widget);
		static void						on_text_field_drag(vekt::builder* b, vekt::id widget, float mp_x, float mp_y, float delta_x, float delta_y, unsigned int button);
		static void						on_text_field_focus_lost(vekt::builder* b, vekt::id widget);
		static void						on_text_field_focus_gained(vekt::builder* b, vekt::id widget, bool from_nav);
		static void						on_context_item_hover_begin(vekt::builder* b, vekt::id widget);
		static void						on_context_item_hover_end(vekt::builder* b, vekt::id widget);

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
