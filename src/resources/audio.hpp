// Copyright (c) 2025 Inan Evin

#pragma once

#include "resources/common_resources.hpp"
#include "data/bitmask.hpp"
#include "memory/chunk_handle.hpp"
#include "reflection/resource_reflection.hpp"

struct ma_decoder;

namespace SFG
{
	struct audio_raw;
	class world;

	class audio
	{
	public:
		enum flags
		{
			is_streaming = 1 << 0,
			is_created	 = 1 << 1,
		};

		~audio();

		// -----------------------------------------------------------------------------
		// resource
		// -----------------------------------------------------------------------------

		void create_from_loader(const audio_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		ma_decoder* get_decoder(world& w) const;

		inline const bitmask<uint8>& get_flags() const
		{
			return _flags;
		}

	private:
		chunk_handle32 _decoder	   = {};
		chunk_handle32 _audio_data = {};
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
		bitmask<uint8> _flags = 0;
	};

	REGISTER_RESOURCE(audio, "stkaud");
}
