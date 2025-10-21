// Copyright (c) 2025 Inan Evin

#pragma once

#include "resources/common_resources.hpp"
#include "data/bitmask.hpp"
#include "memory/chunk_handle.hpp"
#include "reflection/resource_reflection.hpp"
#include <vendor/miniaudio/miniaudio.h>

namespace SFG
{
	struct audio_raw;
	class world;

	class audio
	{
	public:
		enum flags
		{
			is_init		 = 1 << 0,
			is_streaming = 1 << 1,
		};

		void create_from_loader(const audio_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

		inline const ma_decoder& get_decoder() const
		{
			return _decoder;
		}

		inline const bitmask<uint8>& get_flags() const
		{
			return _flags;
		}

	private:
		ma_decoder _decoder{};
		// ma_sound	   _sound{};
		chunk_handle32 _audio_data = {};
		bitmask<uint8> _flags	   = 0;
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
	};

	REGISTER_RESOURCE(audio, "stkaud");
}
