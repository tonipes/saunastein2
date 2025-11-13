// Copyright (c) 2025 Inan Evin

#include "simple_file_watcher.hpp"
#include "data/vector_util.hpp"
#include "io/log.hpp"
#include "io/file_system.hpp"
#include <filesystem>

namespace SFG
{
	void simple_file_watcher::add_path(const char* path, uint16 optional_id)
	{
		if (!file_system::exists(path))
		{
			SFG_ERR("Can't add path to file watcher as it doesn't exist! {0}", path);
			return;
		}

		const uint64 last_modified = file_system::get_last_modified_ticks(path);
		_paths.push_back(new entry(new std::filesystem::path(path), path, last_modified, optional_id));
	}
	void simple_file_watcher::remove_path(const char* path)
	{
		auto it = vector_util::find_if(_paths, [path](entry* e) -> bool { return strcmp(path, e->str.c_str()) == 0; });

		if (it != _paths.end())
		{
			entry* e = *it;
			delete e->path;
			delete e;
		}
	}

	void simple_file_watcher::tick()
	{
		_ticks++;

		if (_ticks > _tick_interval)
		{
			_ticks = 0;
			watch();
		}
	}

	void simple_file_watcher::watch()
	{
		for (entry* e : _paths)
		{
			const uint64 ticks = file_system::get_last_modified_ticks(*e->path);
			if (e->last_modified != ticks)
			{
				e->last_modified = ticks;
				if (_callback)
					_callback(e->str.c_str(), e->last_modified, e->id, _callback_ud);
			}
		}
	}

	void simple_file_watcher::clear()
	{
		for (const entry* p : _paths)
		{
			delete p->path;
			delete p;
		}

		_paths.resize(0);
		_ticks = 0;
	}

} // namespace SFG
