// Copyright (c) 2025 Inan Evin

#include "draw_stream.hpp"
#include "memory/bump_allocator.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"

#include <algorithm>

namespace SFG
{

	void draw_stream::prepare(bump_allocator& alloc, size_t max_commands)
	{
		_commands		= alloc.allocate<draw_command>(max_commands);
		_keys			= alloc.allocate<uint64>(max_commands);
		_indices		= alloc.allocate<uint32>(max_commands);
		_commands_count = 0;
	}

	void draw_stream::build()
	{
		for (uint32 i = 0; i < _commands_count; i++)
		{
			draw_command& cmd	= _commands[i];
			uint64&		  key	= _keys[i];
			uint32&		  index = _indices[i];
			key					= make_sort_key(cmd.pipeline_hw, cmd.vb_hw, cmd.ib_hw);
			index				= i;
		}

		std::stable_sort(_indices, _indices + _commands_count, [&](const uint32& a, const uint32& b) -> bool { return _keys[a] < _keys[b]; });

		for (uint32 i = 0; i < _commands_count; ++i)
		{
			while (_indices[i] != i)
			{
				const uint32 j = _indices[i];
				std::swap(_commands[i], _commands[j]);
				std::swap(_indices[i], _indices[j]);
			}
		}
	}

	void draw_stream::draw(gfx_id cmd_buffer)
	{
		gfx_backend* backend = gfx_backend::get();

		bound_state bound = {};

		for (uint32 i = 0; i < _commands_count; i++)
		{
			draw_command& draw = _commands[i];

			const uint8 diff = diff_mask(draw.pipeline_hw, draw.ib_hw, draw.vb_hw, bound);
			if (diff & 1u)
				backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = draw.pipeline_hw});

			if (diff & 1u << 1)
				backend->cmd_bind_vertex_buffers(cmd_buffer, {.buffer = draw.vb_hw, .vertex_size = draw.vertex_size});

			if (diff & 1u << 2)
				backend->cmd_bind_index_buffers(cmd_buffer, {.buffer = draw.ib_hw, .index_size = static_cast<uint8>(sizeof(primitive_index))});

			backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)&draw.entity_constant_index, .offset = constant_index_object_constant0, .count = 1, .param_index = rpi_constants});

			const uint32 mat_constants[2] = {draw.material_constant_index, draw.texture_constant_index};
			backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)mat_constants, .offset = constant_index_mat_constant0, .count = 2, .param_index = rpi_constants});
			backend->cmd_draw_indexed_instanced(cmd_buffer,
												{
													.index_count_per_instance = draw.index_count,
													.instance_count			  = draw.instance_count,
													.start_index_location	  = draw.start_index,
													.base_vertex_location	  = draw.base_vertex,
													.start_instance_location  = draw.first_instance,
												});
		}
	}
}
