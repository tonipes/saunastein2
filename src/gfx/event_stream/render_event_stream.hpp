// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "data/ostream.hpp"
#include "gfx/event_stream/render_events.hpp"
#include <vendor/moodycamel/readerwriterqueue.h>

namespace SFG
{
	class render_event_stream
	{
	public:
		static constexpr size_t MAX_RENDER_EVENTS = 2048;
		static constexpr size_t BATCH_SIZE		  = 1024 * 1024;
		static constexpr size_t MAX_BATCHES		  = 256;

		struct event_batch
		{
			uint8* data	 = nullptr;
			size_t size	 = 0;
			size_t ident = 0;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void publish();

		// -----------------------------------------------------------------------------
		// event api
		// -----------------------------------------------------------------------------

		template <typename T> inline void add_event(const render_event_header& header, const T& ev)
		{
			header.serialize(_main_thread_data);
			ev.serialize(_main_thread_data);
			SFG_ASSERT(_main_thread_data.get_size() < BATCH_SIZE);
		}

		inline void add_event(const render_event_header& header)
		{
			header.serialize(_main_thread_data);
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline moodycamel::ReaderWriterQueue<event_batch, MAX_BATCHES>& get_events()
		{
			return _event_queue;
		}

	private:
		ostream													_main_thread_data = {};
		moodycamel::ReaderWriterQueue<event_batch, MAX_BATCHES> _event_queue;
		event_batch												_batches[MAX_BATCHES];
		size_t													_batch_counter = 0;
	};
}