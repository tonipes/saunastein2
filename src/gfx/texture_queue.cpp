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
	}

	void texture_queue::uninit()
	{
	}

	void texture_queue::add_request(const static_vector<texture_buffer, MAX_TEXTURE_MIPS>& buffers, gfx_id texture, gfx_id intermediate, uint8 use_free, resource_state state)
	{
		for (texture_request& req : _requests)
		{
			if (req.texture == texture)
			{
				for (texture_buffer& b : req.buffers)
				{
					if (req.use_free)
						SFG_FREE(b.pixels);
					else
						delete[] b.pixels;
				}

				req.buffers.clear();
				req.buffers = buffers;
				return;
			}
		}

		const texture_request req = {
			.buffers	  = buffers,
			.texture	  = texture,
			.intermediate = intermediate,
			.added_frame  = frame_info::get_render_frame(),
			.use_free	  = use_free,
			.to_state	  = state,
		};

		_requests.push_back(req);
	}

	void texture_queue::flush_all(gfx_id cmd_list, vector<barrier>& out_barriers)
	{
		gfx_backend* backend = gfx_backend::get();

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

			out_barriers.push_back({
				.resource	 = buf.texture,
				.flags		 = barrier_flags::baf_is_texture,
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = buf.to_state,
			});

			for (const texture_buffer& b : buf.buffers)
			{
				if (buf.use_free)
					SFG_FREE(b.pixels);
				else
					delete[] b.pixels;
			}
		}

		_requests.resize(0);
	}

	bool texture_queue::empty() const
	{
		return _requests.empty();
	}

} // namespace Lina
