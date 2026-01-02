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

#include "editor_panel_controls.hpp"
#include "math/vector2ui16.hpp"
#include "common/system_info.hpp"
#include "common/string_id.hpp"
#include "memory/memory_tracer.hpp"
#include "editor/editor_theme.hpp"

#include "gui/vekt.hpp"
#include "gui/vekt_gui_builder.hpp"

namespace SFG
{
	void editor_panel_controls::init(vekt::builder* builder)
	{

		vekt::gui_builder gui_builder = vekt::gui_builder(builder);
		gui_builder.style.active_font = editor_theme::get().font_title;
		_widget						  = gui_builder.begin_root();

		gui_builder.add_title("general");
		gui_builder.add_title("stats");
		gui_builder.end_root();

		builder->widget_add_child(builder->get_root(), _widget);
	}

	void editor_panel_controls::uninit(vekt::builder* b)
	{
	}

	void editor_panel_controls::draw(const vector2ui16& window_size, vekt::builder* b)
	{
		b->widget_set_pos_abs(_widget, vector2(30, 60));
		b->widget_set_size_abs(_widget, vector2(700, 1000));
	}
}
