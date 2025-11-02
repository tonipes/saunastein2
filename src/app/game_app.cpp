// Copyright (c) 2025 Inan Evin

#include "game_app.hpp"
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

#include "debug_console.hpp"
#include "world/world.hpp"

#ifdef SFG_TOOLMODE
#include "project/engine_data.hpp"
#include "editor/editor.hpp"
#include "gfx/common/descriptions.hpp"
#endif

namespace SFG
{
	game_app::init_status game_app::init(const vector2ui16& render_target_size)
	{
		SFG_SET_INIT(true);
		SFG_REGISTER_THREAD_MAIN();

		// globals
		debug_console::init();
		time::init();

		// toolmode requires correct engine_data initialization
#ifdef SFG_TOOLMODE
		if (!engine_data::get().init())
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
		_main_window->set_event_callback(std::bind(&game_app::on_window_event, this, std::placeholders::_1));

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
		_renderer = new renderer(*_main_window, *_world, _render_stream);
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
			return init_status::renderer_failed;
		}
		_render_stream.init();

		// editor
#ifdef SFG_TOOLMODE
		_editor = new editor(*this);
		_editor->on_init();
#endif

		// finalize
		_render_joined.store(1);
		kick_off_render();
		SFG_SET_INIT(false);
		return init_status::ok;
	}

	void game_app::uninit()
	{
		// editor
#ifdef SFG_TOOLMODE
		_editor->on_uninit();
		delete _editor;
		_editor = nullptr;
#endif
		// world
		_world->uninit();
		delete _world;
		_world = nullptr;

		// renderer
		join_render();
		_render_stream.publish();
		_renderer->uninit();
		renderer::destroy_bind_layout_global();
		delete _renderer;
		_renderer = nullptr;
		_render_stream.uninit();

		// window
		_main_window->destroy();
		delete _main_window;

		// backend
		gfx_backend* backend = gfx_backend::get();
		backend->uninit();
		delete gfx_backend::s_instance;
		gfx_backend::s_instance = nullptr;

		// globals
#ifdef SFG_TOOLMODE
		engine_data::get().uninit();
#endif
		time::uninit();
		debug_console::uninit();
	}

	void game_app::tick()
	{
		constexpr double ema_fixed_ns	= 16'666'667.0;
		int64			 previous_time	= time::get_cpu_microseconds();
		int64			 accumulator_ns = static_cast<int64>(ema_fixed_ns);

		while (_should_close.load(std::memory_order_acquire) == 0)
		{
			// timing.
			const int64 current_time = time::get_cpu_microseconds();
			const int64 delta_micro	 = current_time - previous_time;
			previous_time			 = current_time;
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
				_renderer->on_window_resize(ws);
				kick_off_render();
			}

			// world timing
			const int64		 FIXED_INTERVAL_NS = static_cast<int64>(ema_fixed_ns);
			const float		 dt_seconds		   = static_cast<float>(static_cast<double>(FIXED_INTERVAL_NS) / 1'000'000'000.0);
			constexpr uint32 MAX_TICKS		   = 4;
			uint32			 ticks			   = 0;

			accumulator_ns += delta_micro * 1000;
			while (accumulator_ns >= FIXED_INTERVAL_NS && ticks < MAX_TICKS)
			{
				accumulator_ns -= FIXED_INTERVAL_NS;
				_world->tick(ws, dt_seconds);
#ifdef SFG_TOOLMODE
				_editor->on_tick(dt_seconds);
#endif
				ticks++;
			}

			// interpolation
			const double interpolation = static_cast<double>(accumulator_ns) / static_cast<double>(FIXED_INTERVAL_NS);
			_world->post_tick(interpolation);

#ifdef SFG_TOOLMODE
			_editor->on_post_tick(interpolation);
			engine_shaders::get().tick();
#endif

			// pipeline render events.
			_render_stream.publish();
			_renderer->tick();
			frame_info::s_frame.fetch_add(1);
		}
	}

	void game_app::on_window_event(const window_event& ev)
	{
		if (ev.type != window_event_type::focus && !_main_window->get_flags().is_set(window_flags::wf_has_focus))
			return;

		if (_renderer && _renderer->on_window_event(ev))
			return;

#ifdef SFG_TOOLMODE
		if (_editor && _editor->on_window_event(ev))
			return;
#endif

		if (_world)
			_world->on_window_event(ev);
	}

	void game_app::join_render()
	{
		if (_render_joined.load() == 1)
			return;

		_render_joined.store(1, std::memory_order_release);
		if (_render_thread.joinable())
			_render_thread.join();

		frame_info::s_is_render_active = false;
		_renderer->wait_backend();
	}

	void game_app::kick_off_render()
	{
		if (_render_joined.load() == 0)
			return;
		_render_joined.store(0, std::memory_order_release);
		_render_thread				   = std::thread(&game_app::render_loop, this);
		frame_info::s_is_render_active = true;
	}

	void game_app::render_loop()
	{
		SFG_REGISTER_THREAD_RENDER();

		while (_render_joined.load(std::memory_order_acquire) == 0)
		{

#ifdef SFG_TOOLMODE
			_editor->on_render();
#endif
			_renderer->render();
			frame_info::s_render_frame.fetch_add(1);

#ifndef SFG_PRODUCTION
			frame_info::s_present_time_milli.store(frame_info::s_render_present_microseconds * 0.001);
			frame_info::s_render_thread_time_milli.store(static_cast<double>(frame_info::s_render_work_microseconds) * 0.001);
			frame_info::s_fps.store(1.0f / static_cast<float>((frame_info::s_present_time_milli + frame_info::s_render_thread_time_milli) * 0.001f));
#endif
		}
	}
}
