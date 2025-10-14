// Copyright (c) 2025 Inan Evin

#include "render_event_stream.hpp"
#include "io/log.hpp"
namespace SFG
{

	void render_event_stream::init()
	{
		_main_thread_data.create(BATCH_SIZE);
	}

	void render_event_stream::uninit()
	{
		_main_thread_data.destroy();
	}

	void render_event_stream::publish()
	{
		if (_main_thread_data.get_size() == 0)
			return;

		event_batch b = {};
		b.size		  = _main_thread_data.get_size();
		SFG_MEMCPY(b.data, _main_thread_data.get_raw(), _main_thread_data.get_size());
		const bool res = _event_queue.try_emplace(b);
		if (res)
		{
		}
		_main_thread_data.shrink(0);
		_main_thread_data.set(0, BATCH_SIZE, 0);
	}
}