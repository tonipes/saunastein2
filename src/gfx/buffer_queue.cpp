// Copyright (c) 2025 Inan Evin

#include "buffer_queue.hpp"
#include "gfx/buffer.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/commands.hpp"

namespace SFG
{

	void buffer_queue::init()
	{
		_requests.reserve(256);
		for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
			_pfd[i].buffered_requests.reserve(256);
	}

	void buffer_queue::uninit()
	{
	}

	void buffer_queue::add_request(const buffer_request& req)
	{
		_requests.push_back(req);
	}

	void buffer_queue::add_request(const update_request& req, uint8 frame_index)
	{
		_pfd[frame_index].buffered_requests.push_back(req);
	}

	void buffer_queue::flush_all(gfx_id cmd_list, uint8 frame_index, vector<barrier>& out_barriers)
	{
		gfx_backend*	backend = gfx_backend::get();
		per_frame_data& pfd		= _pfd[frame_index];

		// perform copies.
		for (const buffer_request& buf : _requests)
		{
			buf.buffer->copy(cmd_list);

			out_barriers.push_back({
				.resource	= buf.buffer->get_hw_gpu(),
				.flags		= barrier_flags::baf_is_resource,
				.from_state = resource_state::copy_dest,
				.to_state	= buf.to_state,
			});
		}

		for (update_request& buf : pfd.buffered_requests)
		{
			buf.buffer->buffer_data(0, buf.data, buf.data_size);
			buf.buffer->copy(cmd_list);
			SFG_FREE(buf.data);

			out_barriers.push_back({
				.resource	= buf.buffer->get_hw_gpu(),
				.flags		= barrier_flags::baf_is_resource,
				.from_state = resource_state::copy_dest,
				.to_state	= buf.to_state,
			});
		}

		_requests.resize(0);
		pfd.buffered_requests.resize(0);
	}

	bool buffer_queue::empty(uint8 frame_index) const
	{
		return _requests.empty() && _pfd[frame_index].buffered_requests.empty();
	}
}
