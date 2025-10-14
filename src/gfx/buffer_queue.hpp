// Copyright (c) 2025 Inan Evin

#pragma once
#include "common/size_definitions.hpp"
#include "data/vector.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/barrier_description.hpp"

namespace SFG
{
	class buffer;

	class buffer_queue
	{
	public:
		struct buffer_request
		{
			buffer*		   buffer	= nullptr;
			resource_state to_state = resource_state::ps_resource;
		};

		struct update_request
		{
			buffer*		   buffer	 = nullptr;
			uint8*		   data		 = nullptr;
			size_t		   data_size = 0;
			resource_state to_state	 = resource_state::ps_resource;
		};

		struct per_frame_data
		{
			vector<update_request> buffered_requests = {};
		};

		void init();
		void uninit();
		void add_request(const buffer_request& req);
		void add_request(const update_request& req, uint8 frame_index);
		void flush_all(gfx_id cmd, uint8 frame_index, vector<barrier>& out_barriers);
		bool empty(uint8 frame_index) const;

	private:
		per_frame_data		   _pfd[BACK_BUFFER_COUNT];
		vector<buffer_request> _requests = {};
	};
} // namespace Lina
