// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/string.hpp"
#include "data/vector.hpp"
#include "common/string_id.hpp"

namespace std
{
	namespace filesystem
	{
		class path;
	}
}

namespace SFG
{

	typedef void (*simple_file_watcher_callback)(const char* p, uint64 last_modified, uint16 id, void* user_data);

	class simple_file_watcher
	{
	private:
		struct entry
		{
			std::filesystem::path* path			 = nullptr;
			string				   str			 = "";
			uint64				   last_modified = 0;
			uint16				   id			 = 0;

			~entry()
			{
			}
		};

	public:
		~simple_file_watcher()
		{
			clear();
		}

		void add_path(const char* path, uint16 optional_id = 0);
		void remove_path(const char* path);
		void clear();
		void tick();

		inline void set_tick_interval(uint16 interval)
		{
			_tick_interval = interval;
		}

		inline void set_callback(simple_file_watcher_callback cb, void* user_data)
		{
			_callback	 = cb;
			_callback_ud = user_data;
		}

		inline void reserve(int count)
		{
			_paths.reserve(count);
		}

	private:
		void watch();

	private:
		simple_file_watcher_callback _callback	  = nullptr;
		void*						 _callback_ud = nullptr;
		vector<entry*>				 _paths;
		uint16						 _tick_interval = 1;
		uint16						 _ticks			= 0;
	};

}
