// Copyright (c) 2025 Inan Evin

#include "render_event_stream.hpp"
#include "io/log.hpp"
#include "data/istream.hpp"
#include <tracy/Tracy.hpp>

namespace SFG
{
	void render_event_stream::init()
	{
		_main_thread_data.create(BATCH_SIZE);

		for (uint32 i = 0; i < MAX_BATCHES; i++)
		{
			_stream_data[i].data = new uint8[BATCH_SIZE];
		}
	}

	void render_event_stream::uninit()
	{
		_main_thread_data.destroy();

		for (uint32 i = 0; i < MAX_BATCHES; i++)
		{
			delete _stream_data[i].data;
			_stream_data[i].data = nullptr;
		}
	}

	void render_event_stream::publish()
	{
		ZoneScoped;

		const size_t sz = _main_thread_data.get_size();
		if (sz == 0)
			return;
		SFG_ASSERT(sz < RENDER_STREAM_BATCH_SIZE);

		const int8 last_rendered = _rendered.load(std::memory_order_acquire);
		const int8 write_index	 = (last_rendered + 1) % RENDER_STREAM_MAX_BATCHES;


		buffered_data& buf = _stream_data[write_index];
		SFG_MEMCPY(buf.data + buf.size, _main_thread_data.get_raw(), _main_thread_data.get_size());
		buf.size += _main_thread_data.get_size();
		SFG_ASSERT(buf.size < RENDER_STREAM_BATCH_SIZE);

		_main_thread_data.shrink(0);
		_latest.store(write_index, std::memory_order_release);

		return;

		// const size_t sz = _main_thread_data.get_size();
		// if (sz == 0)
		// 	return;
		// SFG_ASSERT(sz < BATCH_SIZE);
		//
		// event_batch& b = _batches[_write_index];
		// b.size		   = sz;
		// b.ready.store(true, std::memory_order_release);
		// SFG_MEMCPY(b.data, _main_thread_data.get_raw(), sz);
		//
		// SFG_TRACE("{0}", ++ctr);
		// return;
		// const bool res = _event_queue.try_enqueue(_write_index);
		// _write_index   = (_write_index + 1) % MAX_BATCHES;
		//
		// _main_thread_data.shrink(0);
	}

	void render_event_stream::open_into(istream& stream)
	{
		const int8 last_written = _latest.load(std::memory_order_acquire);
		if (last_written < 0)
			return;

		_rendered.store(last_written, std::memory_order_release);
		buffered_data& buf = _stream_data[last_written];
		stream.open(buf.data, buf.size);
		buf.size = 0;
	}
}