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
#include "memory/text_allocator.hpp"

namespace vekt
{
	class builder;
};

namespace SFG
{
	struct vector2ui16;

	class editor_panel_controls
	{
	public:
		void init(vekt::builder* builder);
		void uninit();

		void draw(const vector2ui16& window_size);

		static vekt::input_event_result on_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase);

	private:
		vekt::builder* _builder	   = nullptr;
		text_allocator _text_alloc = {};
		vekt::id	   _widget	   = NULL_WIDGET_ID;
		vekt::id	   _hyperlink  = NULL_WIDGET_ID;
		vekt::id	   _fps		   = NULL_WIDGET_ID;
		vekt::id	   _main	   = NULL_WIDGET_ID;
		vekt::id	   _render	   = NULL_WIDGET_ID;
		vekt::id	   _ram		   = NULL_WIDGET_ID;
		vekt::id	   _vram	   = NULL_WIDGET_ID;
		vekt::id	   _vram_txt   = NULL_WIDGET_ID;
		vekt::id	   _vram_res   = NULL_WIDGET_ID;
		vekt::id	   _game_res   = NULL_WIDGET_ID;
		vekt::id	   _window_res = NULL_WIDGET_ID;
		vekt::id	   _draw_calls = NULL_WIDGET_ID;
	};
}
