// Copyright (c) 2025 Inan Evin

#include "render_event_stream.hpp"
#include "io/log.hpp"
namespace SFG
{
	void render_event_stream::init()
	{
		_main_thread_data.create(BATCH_SIZE);
		for (size_t i = 0; i < MAX_BATCHES; i++)
			_batches[i].data = new uint8[BATCH_SIZE];
	}

	void render_event_stream::uninit()
	{
		_main_thread_data.destroy();
		for (size_t i = 0; i < MAX_BATCHES; i++)
			delete[] _batches[i].data;
	}

	void render_event_stream::publish()
	{
		if (_main_thread_data.get_size() == 0)
			return;

		const size_t sz = _main_thread_data.get_size();

		event_batch& b = _batches[_batch_counter];
		b.size		   = sz;
		b.ident		   = _batch_counter;

		SFG_ASSERT(sz < BATCH_SIZE);
		SFG_MEMCPY(b.data, _main_thread_data.get_raw(), sz);
		_batch_counter = (_batch_counter + 1) % MAX_BATCHES;

		const bool res = _event_queue.try_enqueue(b);

		_main_thread_data.shrink(0);
		_main_thread_data.set(0, BATCH_SIZE, 0);
	}
}