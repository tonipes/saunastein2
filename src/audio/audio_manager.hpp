// Copyright (c) 2025 Inan Evin

#pragma once

#include "audio_manager.hpp"

struct ma_engine;

namespace SFG
{
	class audio_manager
	{
	public:
		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ma_engine* get_engine() const
		{
			return _engine;
		}

	private:
		ma_engine* _engine = nullptr;
	};

}
