// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "gfx/common/gfx_constants.hpp"

namespace SFG
{
	struct draw_command
	{
		// geometry
		uint32 start_index;
		uint32 index_count;
		uint32 base_vertex;
		uint32 instance_count;
		uint16 first_instance;

		// state bindings
		gpu_index material_constant_index;
		gpu_index texture_constant_index;
		gpu_index entity_constant_index; // entity constant maps to object_constant0

		// state bindings
		gfx_id vb_hw;
		gfx_id ib_hw;
		gfx_id pipeline_hw;

		uint16 vertex_size;
	};

	static_assert(sizeof(draw_command) <= 64, "cache line pls");

	class bump_allocator;

	class draw_stream
	{
	private:
		struct bound_state
		{
			gfx_id vb		= NULL_GFX_ID;
			gfx_id ib		= NULL_GFX_ID;
			gfx_id pipeline = NULL_GFX_ID;
		};

	public:
		void prepare(bump_allocator& alloc, size_t max_commands);
		void build();
		void draw(gfx_id command_buffer);

		inline void add_command(const draw_command& cmd)
		{
			_commands[_commands_count++] = cmd;
		}

	private:
		inline uint64 make_sort_key(gfx_id pipeline, gfx_id vb, gfx_id ib)
		{
			constexpr uint32 PIPE_BITS = 20;
			constexpr uint32 IB_BITS   = 22;
			constexpr uint32 VB_BITS   = 22;
			return ((uint64)pipeline << (IB_BITS + VB_BITS)) | ((uint64)ib << VB_BITS) | ((uint64)vb);
		}

		inline uint8 diff_mask(gfx_id cur_pipe, gfx_id cur_ib, gfx_id cur_vb, bound_state& bound)
		{
			uint8 mask = 0;
			if (cur_pipe != bound.pipeline)
				mask |= 1u;
			if (cur_ib != bound.ib)
				mask |= 1u << 1;
			if (cur_vb != bound.vb)
				mask |= 1u << 2;

			// Update
			if (mask & 1u)
				bound.pipeline = cur_pipe;
			if (mask & (1u << 1))
				bound.ib = cur_ib;
			if (mask & (1u << 2))
				bound.vb = cur_vb;
			return mask;
		}

	private:
		draw_command* _commands		  = nullptr;
		uint64*		  _keys			  = 0;
		uint32*		  _indices		  = 0;
		uint32		  _commands_count = 0;
	};
}
