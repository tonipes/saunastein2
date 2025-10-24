// Copyright (c) 2025 Inan Evin

#pragma once
#include "data/vector.hpp"
#include "common/size_definitions.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/texture_buffer.hpp"
#include "data/static_vector.hpp"
#include "gfx/common/barrier_description.hpp"
#include <functional>

namespace SFG
{
	struct texture_buffer;
	struct barrier;

	class texture_queue
	{
	private:
		struct texture_request
		{
			static_vector<texture_buffer, MAX_TEXTURE_MIPS> buffers;
			gfx_id											texture		 = 0;
			gfx_id											intermediate = 0;
			uint64											added_frame	 = 0;
			uint8											cleared		 = 0;
			uint8											use_free	 = 0;
			resource_state									to_state	 = resource_state::resource_state_ps_resource;
		};

	public:
		void init();
		void uninit();

		void add_request(const static_vector<texture_buffer, MAX_TEXTURE_MIPS>& buffers, gfx_id texture, gfx_id intermediate, uint8 use_free, resource_state state);
		void flush_all(gfx_id cmd, vector<barrier>& out_barriers);
		bool empty() const;

	private:
		vector<texture_request> _requests = {};
	};
} // namespace Lina
