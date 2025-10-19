// Copyright (c) 2025 Inan Evin

#include "common/size_definitions.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "data/binary_semaphore.hpp"
#include "data/atomic.hpp"
#include "data/bitmask.hpp"
#include "math/vector2ui16.hpp"
#include <thread>

namespace SFG
{
	class window;
	struct window_event;
	class world;
	class renderer;
	class editor;

	class game_app
	{
	public:
		enum game_app_flags
		{
			gaf_should_exit = 1 << 0,
		};

		enum init_status : int32
		{
			ok							 = 1,
			working_directory_dont_exist = -1,
		};

		game_app() {

		};

		int32 init(const vector2ui16& render_target_size);
		void  uninit();
		void  tick();
		void  join_render();
		void  set_swapchain_flags(uint8 flags);

		inline bool get_should_exit() const
		{
			return _flags.is_set(gaf_should_exit);
		}

		inline window* get_main_window()
		{
			return _main_window;
		}

		inline window* get_window() const
		{
			return _main_window;
		}

		inline renderer* get_renderer() const
		{
			return _renderer;
		}

		inline world* get_world() const
		{
			return _world;
		}

	private:
		void kick_off_render();
		void render_loop();
		void on_window_event(const window_event& ev);

	private:
		window*				_main_window = nullptr;
		renderer*			_renderer	 = nullptr;
		world*				_world		 = nullptr;
		render_event_stream _render_stream;
		std::thread			_render_thread;
		vector2ui16			_window_size = {};
		atomic<uint8>		_should_close;
		atomic<uint8>		_render_joined;
		bitmask<uint8>		_flags = 0;
#ifdef SFG_TOOLMODE
		editor* _editor = nullptr;
#endif
	};
}
