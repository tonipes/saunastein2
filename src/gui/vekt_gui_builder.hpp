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

namespace vekt
{
	class builder;
	class font;

	class gui_builder
	{
	public:
		gui_builder(vekt::builder* b) : _builder(b) {};

		struct gui_builder_style
		{

			gui_builder_style();
			SFG::vector4 col_title_line_start;
			SFG::vector4 col_title_line_end;
			SFG::vector4 col_hyperlink;

			SFG::vector4 col_title;
			SFG::vector4 col_text;
			SFG::vector4 col_frame_bg;
			SFG::vector4 col_area_bg;
			SFG::vector4 col_root;
			font*		 active_font = nullptr;

			float root_margin;
			float item_spacing;
			float title_line_width;
			float title_line_height;
		};

		gui_builder_style style = {};

		id	 begin_root();
		void end_root();

		id	 begin_area();
		void end_area();

		id add_title(const char* title);
		id add_label(const char* label);
		id add_hyperlink(const char* label, const char* link);

	private:
		id new_widget(bool push_to_stack = false);

		void push_stack(id s);
		id	 pop_stack();
		id	 stack();

	private:
		static constexpr unsigned int STACK_SIZE = 512;

		vekt::builder* _builder			  = nullptr;
		id			   _stack[STACK_SIZE] = {NULL_WIDGET_ID};
		id			   _root			  = NULL_WIDGET_ID;
		id			   _stack_ptr		  = 0;
	};

}
