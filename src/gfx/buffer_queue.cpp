// Copyright (c) 2025 Inan Evin

#include "buffer_queue.hpp"
#include "gfx/buffer.hpp"

namespace SFG
{

	void buffer_queue::init()
	{
		_requests.reserve(256);
		for (uint8 i = 0; i < FRAMES_IN_FLIGHT; i++)
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
		_pfd->buffered_requests.push_back(req);
	}

	void buffer_queue::flush_all(gfx_id cmd_list, uint8 frame_index)
	{
		for (const buffer_request& buf : _requests)
			buf.buffer->copy(cmd_list);

		per_frame_data& pfd = _pfd[frame_index];

		for (const update_request& buf : pfd.buffered_requests)
		{
			buf.buffer->buffer_data(0, buf.data, buf.data_size);
			buf.buffer->copy(cmd_list);
		}

		_requests.resize(0);
		pfd.buffered_requests.resize(0);
	}

	bool buffer_queue::empty(uint8 frame_index) const
	{
		for (const buffer_request& buf : _requests)
		{
			if (buf.buffer->is_dirty())
				return false;
		}

		const per_frame_data& pfd = _pfd[frame_index];

		for (const update_request& buf : pfd.buffered_requests)
		{
			if (buf.buffer->is_dirty())
				return false;
		}

		return true;
	}
} // namespace Lina
