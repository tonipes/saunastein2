// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "data/ostream.hpp"
#include "gfx/event_stream/render_events.hpp"
#include "game/game_max_defines.hpp"

#include "data/atomic.hpp"
#include <vendor/moodycamel/readerwriterqueue.h>

namespace SFG
{
	class istream;

	class render_event_stream
	{
	public:
		static constexpr size_t BATCH_SIZE	= RENDER_STREAM_BATCH_SIZE;
		static constexpr size_t MAX_BATCHES = RENDER_STREAM_MAX_BATCHES;

		struct event_batch
		{
			atomic<bool> ready;
			uint8*		 data = nullptr;
			size_t		 size = 0;
		};

		struct buffered_data
		{
			uint8* data = nullptr;
			size_t size = 0;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void publish();
		void open_into(istream& stream);

		// -----------------------------------------------------------------------------
		// event api
		// -----------------------------------------------------------------------------

		template <typename T> inline void add_event(const render_event_header& header, const T& ev)
		{
			header.serialize(_main_thread_data);
			ev.serialize(_main_thread_data);
			SFG_ASSERT(_main_thread_data.get_size() < RENDER_STREAM_BATCH_SIZE);
		}

		inline void add_event(const render_event_header& header)
		{
			header.serialize(_main_thread_data);
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		// inline moodycamel::ReaderWriterQueue<uint32, MAX_BATCHES>& get_events()
		//{
		//	return _event_queue;
		// }
		//
		// inline event_batch& get_batch(uint32 idx)
		//{
		//	return _batches[idx];
		// }

	private:
		buffered_data _stream_data[RENDER_STREAM_MAX_BATCHES];
		atomic<int8>  _latest			= {-1};
		atomic<int8>  _rendered			= {-1};
		ostream		  _main_thread_data = {};
		// moodycamel::ReaderWriterQueue<uint32, MAX_BATCHES> _event_queue;
		// event_batch										   _batches[MAX_BATCHES];
		// uint32											   _write_index = 0;
	};
}