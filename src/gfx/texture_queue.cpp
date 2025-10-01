// Copyright (c) 2025 Inan Evin

#include "texture_queue.hpp"
#include "data/vector_util.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/common/texture_buffer.hpp"
#include "common/system_info.hpp"

namespace SFG
{

	void texture_queue::init()
	{
		_requests.reserve(256);
		_flushed_requests.reserve(256);
	}

	void texture_queue::uninit()
	{
		clear_flushed_textures();
	}

	void texture_queue::clear_flushed_textures()
	{
		for (const texture_request& r : _flushed_requests)
		{
			for (const texture_buffer& b : r.buffers)
				delete[] b.pixels;
		}

		_flushed_requests.clear();
	}

	void texture_queue::add_request(const static_vector<texture_buffer, MAX_TEXTURE_MIPS>& buffers, gfx_id texture, gfx_id intermediate)
	{
		auto it = vector_util::find_if(_requests, [&](const texture_request& existing) -> bool { return texture == existing.texture; });
		if (it != _requests.end())
		{
			it->buffers = buffers;
			return;
		}

		const texture_request req = {
			.buffers	  = buffers,
			.texture	  = texture,
			.intermediate = intermediate,
			.added_frame  = frame_info::get_render_frame(),
		};

		_requests.push_back(req);
	}

	void texture_queue::flush_all(gfx_id cmd_list)
	{
		gfx_backend* backend = gfx_backend::get();

		uint8 all_clear = 1;
		for (texture_request& r : _flushed_requests)
		{
			if (r.cleared == 1)
				continue;

			if (r.added_frame + FRAMES_IN_FLIGHT <= frame_info::get_render_frame())
			{
				all_clear = 0;
				continue;
			}

			for (const texture_buffer& b : r.buffers)
				delete[] b.pixels;
			r.cleared = 1;
		}

		if (all_clear)
			_flushed_requests.resize(0);

		for (texture_request& buf : _requests)
		{
			backend->cmd_copy_buffer_to_texture(cmd_list,
												{
													.textures			 = buf.buffers.data(),
													.destination_texture = buf.texture,
													.intermediate_buffer = buf.intermediate,
													.mip_levels			 = static_cast<uint8>(buf.buffers.size()),
													.destination_slice	 = 0,

												});
		}

		_flushed_requests.insert(_flushed_requests.end(), std::make_move_iterator(_requests.begin()), std::make_move_iterator(_requests.end()));

		_requests.resize(0);
	}

	bool texture_queue::empty() const
	{
		return _requests.empty();
	}

} // namespace Lina
