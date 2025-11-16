// Copyright (c) 2025 Inan Evin

#include "audio_manager.hpp"
#include "io/log.hpp"
#include "io/assert.hpp"

#include <vendor/miniaudio/miniaudio.h>

namespace SFG
{
	void audio_manager::init()
	{
		_engine = new ma_engine();

		const ma_result result = ma_engine_init(nullptr, _engine);
		SFG_ASSERT(result == MA_SUCCESS);
		SFG_INFO("Audio engine initialized.");

		ma_device* device = ma_engine_get_device(_engine);
		if (device == nullptr)
		{
			SFG_ERR("Failed to retrieve playback device from engine!");
			return;
		}

		SFG_INFO("[Audio] Playback Device Info:");
		SFG_INFO("  Name:           {0}", device->playback.name);
	}

	void audio_manager::uninit()
	{
		ma_engine_uninit(_engine);
		delete _engine;
		_engine = nullptr;
	}
}
