// Copyright (c) 2025 Inan Evin
#include "game.hpp"
#include "platform/window.hpp"
#include "gfx/renderer.hpp"
#include "platform/time.hpp"
#include "platform/process.hpp"
#include "memory/memory_tracer.hpp"
#include "common/system_info.hpp"
#include "gfx/common/render_data.hpp"
#include "gfx/backend/backend.hpp"
#include "io/log.hpp"
#include "debug_console.hpp"
#include "world/world.hpp"
#include "gfx/world/world_renderer.hpp"

#ifdef SFG_TOOLMODE
#include "io/file_system.hpp"
#include "project/engine_data.hpp"
#endif

namespace SFG
{

	void game_app::init(const vector2ui16& render_target_size)
	{

		SET_INIT(true);
		REGISTER_THREAD_MAIN();

		time::init();
		debug_console::init();

		_render_stream.init();
		_world = new world(_render_stream);

		_main_window = new window();
		_main_window->create("Game", window_flags::wf_style_borderless, vector2i16(0, 0), render_target_size);
		_window_size = render_target_size;

		vector<monitor_info> out_infos;
		process::get_all_monitors(out_infos);
		//_main_window->set_position(out_infos[1].position);
		_main_window->set_size(out_infos[0].size);

		gfx_backend::s_instance = new gfx_backend();
		gfx_backend* backend	= gfx_backend::get();
		backend->init();

		_renderer = new renderer();
		_renderer->init(_main_window, _world);

#ifdef SFG_TOOLMODE

		engine_data::get().init();

		_world->init();

		// const string& last_world = engine_data::get().get_last_world();
		// if (file_system::exists(last_world.c_str()))
		// 	_world->load(last_world.c_str());
		// else
		// {
		// 	engine_data::get().set_last_world("");
		// 	engine_data::get().save();
		// 	_world->load("");
		// }

		/*************** CONSOLE *************/
		debug_console::get()->register_console_function("app_new_world", [this]() {
			join_render();
			_world->load("");
			kick_off_render();
		});

		debug_console::get()->register_console_function<const char*>("app_save_world", [this](const char* path) {
			const string p = engine_data::get().get_working_dir() + path;
			_world->save(p.c_str());
			engine_data::get().set_last_world(p);
			engine_data::get().save();
		});

		debug_console::get()->register_console_function<const char*>("app_load_world", [this](const char* path) {
			join_render();
			const string p = engine_data::get().get_working_dir() + path;
			_world->load(p.c_str());
			kick_off_render();

			engine_data::get().set_last_world(p);
			engine_data::get().save();
		});

#endif

		/*************** DEBUG *************/
		_world->load_debug();
		/*************** DEBUG *************/

		_render_joined.store(1);
		kick_off_render();

		SET_INIT(false);
	}

	void game_app::uninit()
	{
		frame_info::s_is_render_active = false;
		_world->uninit();
		delete _world;

#ifdef SFG_TOOLMODE
		engine_data::get().uninit();
#endif
		join_render();

		time::uninit();
		debug_console::uninit();

		_render_stream.publish();
		_renderer->uninit(_render_stream);
		delete _renderer;

		_main_window->destroy();
		delete _main_window;

		gfx_backend* backend = gfx_backend::get();
		backend->uninit();
		delete gfx_backend::s_instance;
		gfx_backend::s_instance = nullptr;
		_render_stream.uninit();
	}

	void game_app::tick()
	{
		// Default to ~16.666 ms; adapt to measured vsync interval for smoother pacing.
		double ema_fixed_ns	  = 16'666'667.0;
		int64  previous_time  = time::get_cpu_microseconds();
		int64  accumulator_ns = static_cast<int64>(ema_fixed_ns);

		while (_should_close.load(std::memory_order_acquire) == 0)
		{
			const int64 current_time = time::get_cpu_microseconds();
			const int64 delta_micro	 = current_time - previous_time;
			previous_time			 = current_time;
			frame_info::s_main_thread_time_milli.store(static_cast<double>(delta_micro) * 0.001);

			process::pump_os_messages();

			const uint32	  event_count  = _main_window->get_event_count();
			bitmask<uint8>&	  window_flags = _main_window->get_flags();
			window_event*	  events	   = _main_window->get_events();
			const vector2ui16 ws		   = _main_window->get_size();

			if (window_flags.is_set(window_flags::wf_close_requested))
			{
				_should_close.store(1);
				break;
			}

			for (uint32 i = 0; i < event_count; i++)
			{
				const window_event& ev = events[i];
				on_window_event(ev);
			}

			_main_window->clear_events();

			if (window_flags.is_set(window_flags::wf_size_dirty))
			{
				window_flags.remove(window_flags::wf_size_dirty);
				join_render();
				_renderer->on_window_resize(ws);
				_window_size = ws;
				kick_off_render();
			}
			/* add any fast tick events here */

			// Smoothly adapt fixed step to measured present interval to avoid beat against 59.94 Hz.
			{
				const double present_us = frame_info::get_present_time_micro();
				if (present_us > 0.0)
				{
					const double present_ns = present_us * 1000.0;
					// EMA smoothing (alpha=0.1)
					ema_fixed_ns = ema_fixed_ns * 0.9 + present_ns * 0.1;
				}
			}

			const int64 FIXED_INTERVAL_NS = static_cast<int64>(ema_fixed_ns);
			const float dt_seconds		  = static_cast<float>(static_cast<double>(FIXED_INTERVAL_NS) / 1'000'000'000.0);

			constexpr uint32 MAX_TICKS = 4;
			uint32			 ticks	   = 0;

			accumulator_ns += delta_micro * 1000; // micro -> nano
			while (accumulator_ns >= FIXED_INTERVAL_NS && ticks < MAX_TICKS)
			{
				accumulator_ns -= FIXED_INTERVAL_NS;
				_world->tick(ws, dt_seconds);
				ticks++;
			}

			const double interpolation = static_cast<double>(accumulator_ns) / static_cast<double>(FIXED_INTERVAL_NS);
			_world->post_tick(interpolation);
			_render_stream.publish();
			_renderer->tick();
			frame_info::s_frame.fetch_add(1);
		}
	}

	void game_app::on_window_event(const window_event& ev)
	{
		if (_renderer->on_window_event(ev))
			return;

		_world->on_window_event(ev);
	}

	void game_app::join_render()
	{
		REGISTER_THREAD_MAIN();

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
		const vector2ui16& screen_size = _window_size;
		REGISTER_THREAD_RENDER();

		int64 previous_time = time::get_cpu_microseconds();

		while (_render_joined.load(std::memory_order_acquire) == 0)
		{
#ifndef SFG_PRODUCTION
			const int64 current_time = time::get_cpu_microseconds();
			const int64 delta_micro	 = current_time - previous_time;
			previous_time			 = current_time;
#endif

			_world->pre_render(screen_size);
			_renderer->render(_render_stream, screen_size);
			frame_info::s_render_frame.fetch_add(1);

#ifndef SFG_PRODUCTION
			frame_info::s_present_time_milli.store(frame_info::get_present_time_micro() * 0.001);
			frame_info::s_render_thread_time_milli.store(static_cast<double>(delta_micro - frame_info::s_present_time_micro) * 0.001);
			frame_info::s_fps.store(1.0f / static_cast<float>(delta_micro * 1e-6));
#endif
		}
	}
}
