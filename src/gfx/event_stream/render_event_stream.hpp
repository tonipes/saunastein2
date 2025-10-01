// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "gfx/event_stream/render_events.hpp"
#include "vendor/moodycamel/readerwriterqueue.h"

namespace SFG
{

	class render_event_stream
	{
	public:
		static constexpr size_t MAX_RENDER_EVENTS = 512;

		void add_event(const render_event& ev);

		inline moodycamel::ReaderWriterQueue<render_event, MAX_RENDER_EVENTS>& get_events()
		{
			return _events;
		}

	private:
		moodycamel::ReaderWriterQueue<render_event, MAX_RENDER_EVENTS> _events;
	};
}