// Copyright (c) 2025 Inan Evin

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
