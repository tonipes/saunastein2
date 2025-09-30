// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/string.hpp"
#include "data/vector.hpp"
#include "common/string_id.hpp"
#include <functional>

namespace SFG
{

	typedef std::function<void(const char* p, string_id last_modified, uint16 id)> simple_file_watcher_callback;

	class simple_file_watcher
	{

	private:
		struct entry
		{
			string	  path			= "";
			string_id last_modified = 0;
			uint16	  id			= 0;
		};

	public:
		void add_path(const char* path, uint16 optional_id = 0);
		void remove_path(const char* path);
		void clear();
		void tick();

		inline void set_tick_interval(uint16 interval)
		{
			_tick_interval = interval;
		}

		inline void set_callback(simple_file_watcher_callback&& cb)
		{
			_callback = std::move(cb);
		}

		inline void reserve(int count)
		{
			_paths.reserve(count);
		}

	private:
		void watch();

	private:
		vector<entry>				 _paths;
		uint16						 _tick_interval = 1;
		uint16						 _ticks			= 0;
		simple_file_watcher_callback _callback		= nullptr;
	};

}
