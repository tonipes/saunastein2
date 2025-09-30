// Copyright (c) 2025 Inan Evin

#include "simple_file_watcher.hpp"
#include "data/vector_util.hpp"
#include "io/file_system.hpp"
#include "io/log.hpp"

namespace SFG
{
	void simple_file_watcher::add_path(const char* path, uint16 optional_id)
	{
		if (!file_system::exists(path))
		{
			SFG_ERR("Can't add path to file watcher as it doesn't exist! {0}", path);
			return;
		}

		const string_id last_modified = TO_SID(file_system::get_last_modified_date(path));
		_paths.push_back({path, last_modified, optional_id});
	}
	void simple_file_watcher::remove_path(const char* path)
	{
		auto it = vector_util::find_if(_paths, [path](const entry& e) -> bool { return strcmp(path, e.path.c_str()) == 0; });

		if (it != _paths.end())
			_paths.erase(it);
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
		for (entry& e : _paths)
		{
			const string	date = file_system::get_last_modified_date(e.path.c_str());
			const string_id sid	 = TO_SID(date);
			if (e.last_modified != sid)
			{
				e.last_modified = sid;
				if (_callback)
					_callback(e.path.c_str(), e.last_modified, e.id);
			}
		}
	}

	void simple_file_watcher::clear()
	{
		_paths.resize(0);
		_ticks = 0;
	}

} // namespace SFG
