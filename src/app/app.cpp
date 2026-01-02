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

#ifdef TRACY_ENABLE
#include <TracyClient.cpp>
#endif

#include "app.hpp"
#include "common/system_info.hpp"
#include "memory/memory_tracer.hpp"
#include "io/log.hpp"

// platform
#include "platform/time.hpp"
#include "platform/window.hpp"
#include "platform/process.hpp"

// gfx
#include "gfx/renderer.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/engine_shaders.hpp"

// game
#include "game/game.hpp"
#include "game/app_defines.hpp"
#include "game/gameplay.hpp"

#include "debug_console.hpp"
#include "world/world.hpp"

#ifdef SFG_TOOLMODE
#include "editor/editor_settings.hpp"
#include "editor/editor.hpp"
#include "gfx/common/descriptions.hpp"
#endif

namespace SFG
{
	app::init_status app::init(const vector2ui16& render_target_size)
	{
		SFG_SET_INIT(true);
		SFG_REGISTER_THREAD_MAIN();

		// globals
		debug_console::init();
		time::init();

		// toolmode requires correct engine_data initialization
#ifdef SFG_TOOLMODE
		if (!editor_settings::get().init())
		{
			time::uninit();
			debug_console::uninit();
			return init_status::working_directory_dont_exist;
		}
#endif

		// window
		_main_window = new window();
		if (!_main_window->create("Game", window_flags::wf_style_windowed | window_flags::wf_high_freq, vector2i16(0, 0), render_target_size))
		{
			time::uninit();
			debug_console::uninit();

			delete _main_window;
			return init_status::window_failed;
		}
		_main_window->set_event_callback(on_window_event, this);
		editor_settings::get().init_window_layout(*_main_window);

		// backend
		gfx_backend::s_instance = new gfx_backend();
		gfx_backend* backend	= gfx_backend::get();
		if (!backend->init())
		{
			time::uninit();
			debug_console::uninit();

			_main_window->destroy();
			delete _main_window;

			delete backend;
			gfx_backend::s_instance = nullptr;

			return init_status::backend_failed;
		}

		// world
		_world = new world(_render_stream);

		// kick off system shaders.
		renderer::create_bind_layout_global();

		if (!engine_shaders::get().init(renderer::get_bind_layout_global(), renderer::get_bind_layout_global_compute(), this))
		{
			time::uninit();
			debug_console::uninit();

			_main_window->destroy();
			delete _main_window;

			renderer::destroy_bind_layout_global();

			backend->uninit();
			delete backend;

			delete _world;
			delete _renderer;

			return init_status::engine_shaders_failed;
		}

// renderer
#ifdef SFG_TOOLMODE
		_editor	  = new editor(*this);
		_renderer = new renderer(*_main_window, *_world, _render_stream, _editor);
#else
		_renderer = new renderer(*_main_window, *_world, _render_stream, nullptr);
#endif
		if (!_renderer->init())
		{
			time::uninit();
			debug_console::uninit();

			_main_window->destroy();
			delete _main_window;

			backend->uninit();
			delete backend;

			delete _world;
			delete _renderer;

#ifdef SFG_TOOLMODE
			delete _editor;
#else
#endif

			return init_status::renderer_failed;
		}

		_render_stream.init();

		_gameplay = new gameplay();

#ifdef SFG_TOOLMODE
		_editor->init();
#else
		_game = new game(*this);
		_game->init();
#endif

		// finalize
		_render_joined.store(1);
		kick_off_render();

		SFG_SET_INIT(false);
		return init_status::ok;
	}

	void app::uninit()
	{
		// world
		_world->uninit();
		delete _world;
		_world = nullptr;

		// renderer
		join_render();

#ifdef SFG_TOOLMODE
		_editor->uninit();
		delete _editor;
		_editor = nullptr;
#else
		_game->uninit();
		delete _game;
		_game = nullptr;
#endif

		_render_stream.publish();
		_renderer->uninit();
		renderer::destroy_bind_layout_global();
		delete _renderer;
		_renderer = nullptr;
		_render_stream.uninit();

		delete _gameplay;
		_gameplay = nullptr;

		// window
		_main_window->destroy();
		delete _main_window;

		// backend
		gfx_backend* backend = gfx_backend::get();
		backend->uninit();
		delete gfx_backend::s_instance;
		gfx_backend::s_instance = nullptr;

#ifdef SFG_TOOLMODE
		editor_settings::get().uninit();
#endif

		time::uninit();
		debug_console::uninit();
	}

	void app::tick()
	{
#ifdef TRACY_ENABLE
		tracy::SetThreadName("main_thread");
#endif

		constexpr double ema_fixed_ns	= 16'666'667.0;
		int64			 previous_time	= time::get_cpu_microseconds();
		int64			 accumulator_ns = static_cast<int64>(ema_fixed_ns);

		while (_should_close.load(std::memory_order_acquire) == 0)
		{
#ifdef TRACY_ENABLE
			TracyCFrameMarkNamed("main_frame");
#endif

			// timing.
			const int64 _current_time = time::get_cpu_microseconds();
			const int64 delta_micro	  = _current_time - previous_time;
			previous_time			  = _current_time;
			frame_info::s_main_thread_time_milli.store(static_cast<double>(delta_micro) * 0.001);

			// OS & window.
			process::pump_os_messages();
			const bitmask<uint16>& window_flags = _main_window->get_flags();

			if (window_flags.is_set(window_flags::wf_close_requested))
			{
				_should_close.store(1);
				break;
			}

			const vector2ui16 ws = _main_window->get_size();
			if (window_flags.is_set(window_flags::wf_size_dirty))
			{
				_main_window->set_size_dirty(false);
				join_render();

#ifdef SFG_TOOLMODE
				if (ws.x > 64 && ws.y > 64)
				{
					editor_settings::get().window_size = ws;
					editor_settings::get().save_last();
				}
#endif
				_renderer->on_window_resize(ws);
				kick_off_render();
			}

#ifdef SFG_TOOLMODE
			if (window_flags.is_set(window_flags::wf_pos_dirty))
			{
				editor_settings::get().window_pos = vector2(_main_window->get_position().x, _main_window->get_position().y);
				editor_settings::get().save_last();
			}
#endif

#if FIXED_FRAMERATE_ENABLED

			const float dt_seconds = FIXED_FRAMERATE_S;
			uint32		ticks	   = 0;
			accumulator_ns += delta_micro * 1000;
			while (accumulator_ns >= FIXED_FRAMERATE_NS && ticks < FIXED_FRAMERATE_MAX_TICKS)
			{
				accumulator_ns -= FIXED_FRAMERATE_NS;

#ifdef SFG_TOOLMODE
				_editor->pre_world_tick(dt_seconds);
#else
				_game->pre_world_tick(dt_seconds);
#endif
				_world->tick(ws, dt_seconds);

#ifdef SFG_TOOLMODE
				_editor->post_world_tick(dt_seconds);
#else
				_game->post_world_tick(dt_seconds);
#endif

				ticks++;
			}

			if (ticks != 0)
				_world->calculate_abs_transforms();

			// interpolation
#if FIXED_FRAMERATE_USE_INTERPOLATION
			const double interpolation = static_cast<double>(accumulator_ns) / FIXED_FRAMERATE_NS_D;
			_world->interpolate(interpolation);
#endif

#else
			const float dtt = static_cast<float>(static_cast<double>(delta_micro) * 1e-6);

#ifdef SFG_TOOLMODE
			_editor->pre_world_tick(ws, dt_seconds);
#else
			_game->pre_world_tick(ws, dt_seconds);
#endif

			_world->tick(ws, dtt);

#ifdef SFG_TOOLMODE
			_editor->post_world_tick(ws, dt_seconds);
#else
			_game->post_world_tick(ws, dt_seconds);
#endif
			_world->calculate_abs_transforms();
#endif

#ifdef SFG_TOOLMODE
			engine_shaders::get().tick();
#endif

#ifdef SFG_TOOLMODE
			_editor->tick();
#else
			_game->tick();
#endif

			// pipeline render events.
			_render_stream.publish();
			_renderer->tick();
			frame_info::s_frame.fetch_add(1);
		}
	}

	void app::on_window_event(const window_event& ev, void* user_data)
	{
		app* application = static_cast<app*>(user_data);

		if (ev.type != window_event_type::focus && !application->_main_window->get_flags().is_set(window_flags::wf_has_focus))
			return;

		if (application->_renderer && application->_renderer->on_window_event(ev))
			return;

#ifdef SFG_TOOLMODE
		if (application->_editor && application->_editor->on_window_event(ev))
			return;
#endif

		if (application->_world && application->_world->on_window_event(ev, application->_main_window))
			return;
	}

	void app::join_render()
	{
		if (_render_joined.load() == 1)
			return;

		_render_joined.store(1, std::memory_order_release);
		if (_render_thread.joinable())
			_render_thread.join();

		frame_info::s_is_render_active = false;
		_renderer->wait_backend();
	}

	void app::kick_off_render()
	{
		if (_render_joined.load() == 0)
			return;
		_render_joined.store(0, std::memory_order_release);
		_render_thread				   = std::thread(&app::render_loop, this);
		frame_info::s_is_render_active = true;
	}

	void app::render_loop()
	{
		SFG_REGISTER_THREAD_RENDER();
#ifdef TRACY_ENABLE
		tracy::SetThreadName("render_thread");
#endif

		int64 time_prev = time::get_cpu_microseconds();

		while (_render_joined.load(std::memory_order_acquire) == 0)
		{
#ifdef TRACY_ENABLE
			TracyCFrameMarkNamed("render_frame");
#endif

#ifndef SFG_PRODUCTION

			const int64 time_now = time::get_cpu_microseconds();
			const int64 delta	 = time_now - time_prev;
			time_prev			 = time_now;

			const double delta_milli   = delta * 0.001;
			const double delta_seconds = delta_milli * 0.001;

			frame_info::s_render_thread_time_milli.store(delta_milli);
			frame_info::s_render_thread_elapsed_seconds.store(frame_info::s_render_thread_elapsed_seconds.load() + delta_seconds);
			frame_info::s_fps.store(1.0f / static_cast<float>(delta_seconds));
#endif

			_renderer->render();

			frame_info::s_render_frame.fetch_add(1);
		}
	}
}
