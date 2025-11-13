// Copyright (c) 2025 Inan Evin
#pragma once

#include "window_common.hpp"

#ifdef SFG_PLATFORM_WINDOWS
struct HWND__;
#endif
namespace SFG
{

	class window
	{
	private:
	public:
		typedef void (*event_callback)(const window_event& ev, void* user_data);

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		bool create(const char* title, uint16 flags, const vector2i16& pos, const vector2ui16& size);
		void destroy();

		// -----------------------------------------------------------------------------
		// window api
		// -----------------------------------------------------------------------------

		void set_position(const vector2i16& pos);
		void set_size(const vector2ui16& size);
		void set_style(window_flags flags);
		void bring_to_front();
		void confine_cursor(cursor_confinement conf);
		void set_cursor_visible(bool vis);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		static bool is_key_down(uint16 key);

		inline const vector2i16 get_position() const
		{
			return _position;
		}

		inline const vector2ui16 get_size() const
		{
			return _size;
		}

		inline void* get_window_handle() const
		{
			return _window_handle;
		}

		inline void* get_platform_handle() const
		{
			return _platform_handle;
		}

		inline const bitmask<uint16>& get_flags() const
		{
			return _flags;
		}

		inline const monitor_info& get_monitor_info() const
		{
			return _monitor_info;
		}

		inline void set_event_callback(event_callback cb, void* user_data)
		{
			_event_callback			  = cb;
			_event_callback_user_data = user_data;
		}

		inline void set_size_dirty(bool dirty)
		{
			_flags.set(window_flags::wf_size_dirty, dirty);
		}

#ifdef SFG_PLATFORM_WINDOWS
		static __int64 wnd_proc(HWND__* hwnd, unsigned int msg, unsigned __int64 wParam, __int64 lParam);
#endif

	private:
		void add_event(const window_event& ev);

	private:
		event_callback	_event_callback			  = nullptr;
		void*			_event_callback_user_data = nullptr;
		monitor_info	_monitor_info			  = {};
		void*			_window_handle			  = nullptr;
		void*			_platform_handle		  = nullptr;
		int				_prev_confinement[4];
		vector2i16		_mouse_position		= vector2i16::zero;
		vector2i16		_mouse_position_abs = vector2i16::zero;
		vector2i16		_position			= vector2i16::zero;
		vector2ui16		_true_size			= vector2ui16::zero;
		vector2ui16		_size				= vector2ui16::zero;
		bitmask<uint16> _flags				= 0;
		static uint8	s_key_down_map[512];
	};

}