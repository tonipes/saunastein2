// Copyright (c) 2025 Inan Evin

#include "common/size_definitions.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "data/atomic.hpp"
#include "data/bitmask.hpp"
#include "math/vector2ui16.hpp"
#include <thread>

namespace SFG
{
	struct window_event;
	class window;
	class world;
	class renderer;
	class game;
	class editor;

	class app
	{
	public:
		enum class game_app_flags
		{
			should_exit = 1 << 0,
		};

		enum class init_status
		{
			ok,
			working_directory_dont_exist,
			renderer_failed,
			window_failed,
			backend_failed,
			engine_shaders_failed,
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		init_status init(const vector2ui16& render_target_size);
		void		uninit();
		void		tick();
		void		join_render();
		void		kick_off_render();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline bool get_should_exit() const
		{
			return _flags.is_set(static_cast<uint8>(game_app_flags::should_exit));
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

		inline world& get_world() const
		{
			return *_world;
		}

		#ifdef SFG_TOOLMODE
		inline editor* get_editor() const
		{
			return _editor;
	}
		#endif

	private:
		void		render_loop();
		static void on_window_event(const window_event& ev, void* user_data);

	private:
		window*	  _main_window = nullptr;
		renderer* _renderer	   = nullptr;
		world*	  _world	   = nullptr;
		game*	  _game		   = nullptr;

#ifdef SFG_TOOLMODE
		editor* _editor = nullptr;
#endif

		render_event_stream _render_stream;
		std::thread			_render_thread;
		atomic<uint8>		_should_close;
		atomic<uint8>		_render_joined;
		bitmask<uint8>		_flags = 0;
	};
}
