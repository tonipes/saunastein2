// Copyright (c) 2025 Inan Evin

#include "draw_stream.hpp"
#include "memory/bump_allocator.hpp"
#include "math/math.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "resources/vertex.hpp"
#include "gui/vekt.hpp"
#include <algorithm>
#include <tracy/Tracy.hpp>

namespace SFG
{
	void draw_stream::prepare(bump_allocator& alloc, size_t max_commands)
	{
		_commands		= alloc.allocate<draw_command>(max_commands);
		_commands_count = 0;
		_max_commands	= max_commands;
	}

	void draw_stream::build()
	{
		ZoneScoped;

		std::stable_sort(_commands, _commands + _commands_count, [&](auto const& A, auto const& B) {
			return A.priority < B.priority && draw_stream_bound_state::make_sort_key(A.pipeline_hw, A.vb_hw, A.ib_hw) < draw_stream_bound_state::make_sort_key(B.pipeline_hw, B.vb_hw, B.ib_hw);
		});
	}

	void draw_stream::draw(gfx_id cmd_buffer)
	{
		ZoneScoped;

		gfx_backend* backend = gfx_backend::get();

		draw_stream_bound_state bound = {};

		for (uint32 i = 0; i < _commands_count; i++)
		{
			draw_command& draw = _commands[i];

			const uint8 diff = bound.diff_mask(draw.pipeline_hw, draw.ib_hw, draw.vb_hw);
			if (diff & 1u)
				backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = draw.pipeline_hw});

			if (diff & 1u << 1)
				backend->cmd_bind_vertex_buffers(cmd_buffer, {.buffer = draw.vb_hw, .vertex_size = draw.vertex_size});

			if (diff & 1u << 2)
				backend->cmd_bind_index_buffers(cmd_buffer, {.buffer = draw.ib_hw, .index_size = static_cast<uint8>(sizeof(primitive_index))});

			backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)&draw.entity_constant_index, .offset = constant_index_object_constant0, .count = 1, .param_index = rpi_constants});
			backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)&draw.bone_constant_index, .offset = constant_index_object_constant1, .count = 1, .param_index = rpi_constants});

#ifdef SFG_TOOLMODE
			backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)&draw.entity_world_id, .offset = constant_index_object_constant2, .count = 1, .param_index = rpi_constants});
#endif

			const uint32 mat_constants[3] = {draw.material_constant_index, draw.texture_constant_index, draw.sampler_constant_index};
			backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)mat_constants, .offset = constant_index_mat_constant0, .count = 3, .param_index = rpi_constants});
			backend->cmd_draw_indexed_instanced(cmd_buffer,
												{
													.index_count_per_instance = draw.index_count,
													.instance_count			  = 1,
													.start_index_location	  = draw.start_index,
													.base_vertex_location	  = draw.base_vertex,
													.start_instance_location  = 0,
												});
		}
	}

	void draw_stream::add_command(const draw_command& cmd)
	{
		SFG_ASSERT(_commands_count <= _max_commands);
		_commands[_commands_count++] = cmd;
	}

	void draw_stream_distance::prepare(bump_allocator& alloc, size_t max_commands)
	{
		_commands		= alloc.allocate<draw_command_distance>(max_commands);
		_commands_count = 0;
		_max_commands	= max_commands;
	}

	void draw_stream_distance::build()
	{
		ZoneScoped;

		std::stable_sort(_commands, _commands + _commands_count, [&](auto const& A, auto const& B) {
			const float da = A.distance, db = B.distance;
			if (A.priority == B.priority)
			{
				if (math::almost_equal(da, db))
					return draw_stream_bound_state::make_sort_key(A.pipeline_hw, A.vb_hw, A.ib_hw) < draw_stream_bound_state::make_sort_key(B.pipeline_hw, B.vb_hw, B.ib_hw);
				if (math::is_nan(da))
					return false;
				if (math::is_nan(db))
					return true;
				return da > db; // farther first
			}
			return A.priority < B.priority;
		});
	}

	void draw_stream_distance::draw(gfx_id cmd_buffer)
	{
		ZoneScoped;

		gfx_backend* backend = gfx_backend::get();

		draw_stream_bound_state bound = {};

		for (uint32 i = 0; i < _commands_count; i++)
		{
			draw_command_distance& draw = _commands[i];

			const uint8 diff = bound.diff_mask(draw.pipeline_hw, draw.ib_hw, draw.vb_hw);
			if (diff & 1u)
				backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = draw.pipeline_hw});

			if (diff & 1u << 1)
				backend->cmd_bind_vertex_buffers(cmd_buffer, {.buffer = draw.vb_hw, .vertex_size = draw.vertex_size});

			if (diff & 1u << 2)
				backend->cmd_bind_index_buffers(cmd_buffer, {.buffer = draw.ib_hw, .index_size = static_cast<uint8>(sizeof(primitive_index))});

			backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)&draw.entity_constant_index, .offset = constant_index_object_constant0, .count = 1, .param_index = rpi_constants});
			backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)&draw.bone_constant_index, .offset = constant_index_object_constant1, .count = 1, .param_index = rpi_constants});

#ifdef SFG_TOOLMODE
			backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)&draw.entity_world_id, .offset = constant_index_object_constant2, .count = 1, .param_index = rpi_constants});
#endif

			const uint32 mat_constants[3] = {draw.material_constant_index, draw.texture_constant_index, draw.sampler_constant_index};
			backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)mat_constants, .offset = constant_index_mat_constant0, .count = 3, .param_index = rpi_constants});
			backend->cmd_draw_indexed_instanced(cmd_buffer,
												{
													.index_count_per_instance = draw.index_count,
													.instance_count			  = 1,
													.start_index_location	  = draw.start_index,
													.base_vertex_location	  = draw.base_vertex,
													.start_instance_location  = 0,
												});
		}
	}
	void draw_stream_distance::add_command(const draw_command_distance& cmd)
	{
		SFG_ASSERT(_commands_count <= _max_commands);
		_commands[_commands_count++] = cmd;
	}

	void draw_stream_gui::prepare(bump_allocator& alloc, size_t max_commands)
	{
		_commands		= alloc.allocate<draw_command_gui>(max_commands);
		_commands_count = 0;
		_max_commands	= max_commands;
	}

	void draw_stream_gui::build()
	{
		ZoneScoped;

		std::stable_sort(_commands, _commands + _commands_count, [&](auto const& A, auto const& B) { return draw_stream_bound_state::make_sort_key(A.pipeline_hw, A.vb_hw, A.ib_hw) < draw_stream_bound_state::make_sort_key(B.pipeline_hw, B.vb_hw, B.ib_hw); });
	}

	void draw_stream_gui::draw(gfx_id cmd_buffer)
	{
		ZoneScoped;

		gfx_backend* backend = gfx_backend::get();

		draw_stream_bound_state bound = {};

		for (uint32 i = 0; i < _commands_count; i++)
		{
			draw_command_gui& draw = _commands[i];

			const uint8 diff = bound.diff_mask(draw.pipeline_hw, draw.ib_hw, draw.vb_hw);
			if (diff & 1u)
				backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = draw.pipeline_hw});

			if (diff & 1u << 1)
				backend->cmd_bind_vertex_buffers(cmd_buffer, {.buffer = draw.vb_hw, .vertex_size = sizeof(vekt::vertex)});

			if (diff & 1u << 2)
				backend->cmd_bind_index_buffers(cmd_buffer, {.buffer = draw.ib_hw, .index_size = static_cast<uint8>(sizeof(vekt::index))});

			backend->cmd_set_scissors(cmd_buffer, {.x = draw.clip.x, .y = draw.clip.y, .width = draw.clip.z, .height = draw.clip.w});

			const uint32 mat_constants[3] = {draw.material_constant_index, draw.texture_constant_index, draw.font_index};
			backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)mat_constants, .offset = constant_index_mat_constant0, .count = 3, .param_index = rpi_constants});
			backend->cmd_draw_indexed_instanced(cmd_buffer,
												{
													.index_count_per_instance = draw.index_count,
													.instance_count			  = 1,
													.start_index_location	  = draw.start_index,
													.base_vertex_location	  = draw.base_vertex,
													.start_instance_location  = 0,
												});
		}
	}

	void draw_stream_gui::add_command(const draw_command_gui& cmd)
	{
		SFG_ASSERT(_commands_count <= _max_commands);
		_commands[_commands_count++] = cmd;
	}
}
