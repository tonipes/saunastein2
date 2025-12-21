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

#include "world/world_listener.hpp"

namespace SFG
{

	struct vector2ui16;
	struct window_event;

	class app;
	class renderer;
	class editor;
	class window;
	class world;

	class game : public world_listener
	{
	public:
#ifdef SFG_TOOLMODE
		game(app& application, renderer& rend, world& w, editor& edt) : _app(application), _world(w), _renderer(rend), _editor(edt) {};
#else
		game(app& application, renderer& rend, world& w) : _app(application), _world(w), _renderer(rend) {};
#endif
		~game() {};

		// -----------------------------------------------------------------------------
		// game class api
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void pre_tick(float dt);
		void tick(float dt);
		void post_render();
		void on_window_event(const window_event& ev, window* wnd);
		void on_window_resize(const vector2ui16& size);

		// -----------------------------------------------------------------------------
		// world listener api
		// -----------------------------------------------------------------------------

		virtual void on_started_play() override {};
		virtual void on_stopped_play() override {};
		virtual void on_started_physics() override {};
		virtual void on_stopped_physics() override {};

	private:
		app&	  _app;
		world&	  _world;
		renderer& _renderer;

#ifdef SFG_TOOLMODE
		editor& _editor;
#endif
	};

}
