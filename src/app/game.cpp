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
#include "editor/editor.hpp"
#endif

namespace SFG
{

	int32 game_app::init(const vector2ui16& render_target_size)
	{

		SET_INIT(true);
		REGISTER_THREAD_MAIN();

		debug_console::init();
		time::init();

#ifdef SFG_TOOLMODE
		if (!engine_data::get().init())
		{
			time::uninit();
			debug_console::uninit();
			return init_status::working_directory_dont_exist;
		}
#endif

		_render_stream.init();
		_world = new world(_render_stream);

		_main_window = new window();
		_main_window->create("Game", window_flags::wf_style_windowed | window_flags::wf_high_freq, vector2i16(0, 0), render_target_size);
		_main_window->set_event_callback(std::bind(&game_app::on_window_event, this, std::placeholders::_1));
		_window_size = render_target_size;

		vector<monitor_info> out_infos;
		process::get_all_monitors(out_infos);
		//_main_window->set_position(out_infos[1].position);
		//_main_window->set_size(out_infos[0].size);

		gfx_backend::s_instance = new gfx_backend();
		gfx_backend* backend	= gfx_backend::get();
		backend->init();

		_renderer = new renderer();
		_renderer->init(_main_window, _world);

#ifdef SFG_TOOLMODE

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

		debug_console::get()->register_console_function<int, int>("vsync", [this](int v, int tearing) {
			uint8 flags = {};

			if (v == 1)
				flags |= swapchain_flags::sf_vsync_every_v_blank;
			else if (v == 2)
				flags |= swapchain_flags::sf_vsync_every_2v_blank;
			if (tearing == 1)
				flags |= swapchain_flags::sf_allow_tearing;

			SFG_INFO("Setting VSync: {0}, Tearing: {1}", v, tearing);

			set_swapchain_flags(flags);
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

#ifdef SFG_TOOLMODE
		_editor = new editor(*this);
		_editor->on_init();
#endif

		_render_joined.store(1);
		kick_off_render();

		SET_INIT(false);

		return init_status::ok;
	}

	void game_app::uninit()
	{
		frame_info::s_is_render_active = false;
#ifdef SFG_TOOLMODE
		_editor->on_uninit();
		delete _editor;
		_editor = nullptr;
#endif
		_world->uninit();
		delete _world;
		_world = nullptr;

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
		// ~16.666 ms; can adapt later for smoother-pacing
		constexpr double ema_fixed_ns	= 16'666'667.0;
		int64			 previous_time	= time::get_cpu_microseconds();
		int64			 accumulator_ns = static_cast<int64>(ema_fixed_ns);

		while (_should_close.load(std::memory_order_acquire) == 0)
		{
			const int64 current_time = time::get_cpu_microseconds();
			const int64 delta_micro	 = current_time - previous_time;
			previous_time			 = current_time;
			frame_info::s_main_thread_time_milli.store(static_cast<double>(delta_micro) * 0.001);

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
				_window_size = ws;
				kick_off_render();
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
#ifdef SFG_TOOLMODE
				_editor->on_tick(dt_seconds);
#endif
				ticks++;
			}

			const double interpolation = static_cast<double>(accumulator_ns) / static_cast<double>(FIXED_INTERVAL_NS);

			_world->post_tick(interpolation);

#ifdef SFG_TOOLMODE
			_editor->on_post_tick(interpolation);
#endif

			_render_stream.publish();
			_renderer->tick();
			frame_info::s_frame.fetch_add(1);

			// if (ticks == 0)
			//{
			//	const int64 remaining_ns = FIXED_INTERVAL_NS - accumulator_ns;
			//	if (remaining_ns > 0)
			//	{
			//		constexpr int64 MAX_SLEEP_US = 2000;
			//		int64			sleep_us	 = remaining_ns / 1000;
			//		if (sleep_us > MAX_SLEEP_US)
			//			sleep_us = MAX_SLEEP_US;
			//
			//		if (sleep_us > 0)
			//			time::throttle(sleep_us);
			//		else
			//			time::yield_thread();
			//	}
			//	else
			//	{
			//		time::yield_thread();
			//	}
			// }
		}
	}

	void game_app::on_window_event(const window_event& ev)
	{
		if (ev.type != window_event_type::focus && !_main_window->get_flags().is_set(window_flags::wf_has_focus))
			return;

		if (_renderer->on_window_event(ev))
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
		REGISTER_THREAD_MAIN();

		if (_render_joined.load() == 1)
			return;

		_render_joined.store(1, std::memory_order_release);

		if (_render_thread.joinable())
			_render_thread.join();

		frame_info::s_is_render_active = false;
		_renderer->wait_backend();
	}

	void game_app::set_swapchain_flags(uint8 flags)
	{
		join_render();
		_renderer->on_swapchain_flags(flags);
		kick_off_render();
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

#ifdef SFG_TOOLMODE
			_editor->on_pre_render(screen_size);
#endif
			_world->pre_render(screen_size);
			_renderer->render(_render_stream, screen_size);

#ifdef SFG_TOOLMODE
			_editor->on_render();
#endif
			frame_info::s_render_frame.fetch_add(1);

#ifndef SFG_PRODUCTION
			frame_info::s_present_time_milli.store(frame_info::get_present_time_micro() * 0.001);
			frame_info::s_render_thread_time_milli.store(static_cast<double>(delta_micro - frame_info::s_present_time_micro) * 0.001);
			frame_info::s_fps.store(1.0f / static_cast<float>(delta_micro * 1e-6));
#endif
		}
	}
}
