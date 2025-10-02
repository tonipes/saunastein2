// Copyright (c) 2025 Inan Evin

#pragma once
#include "common/size_definitions.hpp"
#include "data/vector.hpp"
#include "gfx/common/gfx_constants.hpp"

namespace SFG
{
	class buffer;

	class buffer_queue
	{
	public:
		struct buffer_request
		{
			buffer* buffer = nullptr;
		};

		struct update_request
		{
			buffer* buffer	  = nullptr;
			uint8*	data	  = nullptr;
			size_t	data_size = 0;
		};

		struct per_frame_data
		{
			vector<update_request> buffered_requests = {};
		};

		void init();
		void uninit();
		void add_request(const buffer_request& req);
		void add_request(const update_request& req, uint8 frame_index);
		void flush_all(gfx_id cmd, uint8 frame_index);
		bool empty(uint8 frame_index) const;

	private:
		per_frame_data		   _pfd[FRAMES_IN_FLIGHT];
		vector<buffer_request> _requests = {};
	};
} // namespace Lina
