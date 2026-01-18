/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "world_debug_rendering.hpp"
#include "gui/vekt.hpp"
#include "memory/memory.hpp"
#include "math/color.hpp"
#include "math/vector2ui16.hpp"
#include "gfx/common/gfx_constants.hpp"

namespace SFG
{
	void world_debug_rendering::init()
	{
		for (uint32 i = 0; i < 3; ++i)
		{
			snapshot& sh	 = _snapshots[i];
			sh.vertices_gui	 = new vertex_gui[MAX_VERTEX_COUNT_GUI];
			sh.vertices_line = new vertex_3d_line[MAX_VERTEX_COUNT_LINE];
			sh.vertices_tri	 = new vertex_simple[MAX_VERTEX_COUNT_TRI];
			sh.indices		 = new primitive_index[MAX_INDEX_COUNT];
			sh.draw_calls	 = new debug_draw_call[MAX_DRAW_CALLS];
		}

		_builder						= new vekt::builder();
		vekt::builder::init_config conf = {};
		_builder->init(conf);

		_published.store(UINT32_MAX, std::memory_order_relaxed);
		_reader_slot.store(UINT32_MAX, std::memory_order_relaxed);
		_writer_slot  = 0;
		_current_read = UINT32_MAX;
	}

	void world_debug_rendering::uninit()
	{
		for (uint32 i = 0; i < 3; ++i)
		{
			snapshot& sh = _snapshots[i];
			delete[] sh.vertices_gui;
			delete[] sh.vertices_line;
			delete[] sh.vertices_tri;
			delete[] sh.indices;
			delete[] sh.draw_calls;
			sh = {};
		}

		if (_builder)
		{
			_builder->uninit();
			delete _builder;
			_builder = nullptr;
		}
	}

	void world_debug_rendering::begin_frame(const vector2ui16& res)
	{
		world_debug_rendering::snapshot& w = _snapshots[_writer_slot];
		w.reset();
		_builder->build_begin(vector2(res.x, res.y));
	}

	void world_debug_rendering::end_frame()
	{
		_builder->build_end();

		const uint32 next = (_writer_slot + 1) % 3;
		_published.store(_writer_slot, std::memory_order_release);
		uint32 cur_reader = _reader_slot.load(std::memory_order_acquire);
		if (cur_reader != UINT32_MAX)
			_reader_slot.store(UINT32_MAX, std::memory_order_release);
		_writer_slot = next;
	}

	void world_debug_rendering::add_indices(const primitive_index* data, uint32 count)
	{
		snapshot&	 s		= _snapshots[_writer_slot];
		const uint32 before = s.idx_count;
		SFG_MEMCPY(&s.indices[before], data, sizeof(primitive_index) * count);
		s.idx_count += count;
	}

	uint32 world_debug_rendering::add_vertex_line(const vertex_3d_line* data, uint32 count)
	{
		snapshot&	 s		= _snapshots[_writer_slot];
		const uint32 before = s.vtx_count_line;
		SFG_MEMCPY(&s.vertices_line[before], data, sizeof(vertex_3d_line) * count);
		s.vtx_count_line += count;
		return before;
	}
	uint32 world_debug_rendering::add_vertex_tri(const vertex_simple* data, uint32 count)
	{
		snapshot&	 s		= _snapshots[_writer_slot];
		const uint32 before = s.vtx_count_tri;
		SFG_MEMCPY(&s.vertices_tri[before], data, sizeof(vertex_simple) * count);
		s.vtx_count_tri += count;
		return before;
	}
	uint32 world_debug_rendering::add_vertex_gui(const vertex_gui* data, uint32 count)
	{
		snapshot&	 s		= _snapshots[_writer_slot];
		const uint32 before = s.vtx_count_gui;
		SFG_MEMCPY(&s.vertices_gui[before], data, sizeof(vertex_gui) * count);
		s.vtx_count_gui += count;
		return before;
	}

	void world_debug_rendering::push_draw_call(uint32 vtx_buffer_index, uint32 start_idx, uint32 idx_count, uint32 base_vtx, uint16 vtx_size)
	{
		snapshot& s = _snapshots[_writer_slot];

		const uint32 before = s.draw_call_count;

		debug_draw_call& dc = s.draw_calls[s.draw_call_count];
		dc.start_index		= start_idx;
		dc.index_count		= idx_count;
		dc.base_vertex		= base_vtx;
		dc.vertex_size		= vtx_size;
		dc.vtx_buffer_index = vtx_buffer_index;
		s.draw_call_count++;
	}

	void world_debug_rendering::draw_line(const vector3& p0, const vector3& p1, const color& col, float thickness)
	{
		const vertex_3d_line v[4] = {
			{.pos = p0, .next_pos = p1, .color = col.to_vector(), .direction = thickness},
			{.pos = p0, .next_pos = p1, .color = col.to_vector(), .direction = -thickness},
			{.pos = p1, .next_pos = p0, .color = col.to_vector(), .direction = thickness},
			{.pos = p1, .next_pos = p0, .color = col.to_vector(), .direction = -thickness},

		};

		const uint32		  base_vtx = add_vertex_line(v, 4);
		const primitive_index idxs[6]  = {
			 (primitive_index)(base_vtx + 0),
			 (primitive_index)(base_vtx + 1),
			 (primitive_index)(base_vtx + 2),
			 (primitive_index)(base_vtx + 2),
			 (primitive_index)(base_vtx + 3),
			 (primitive_index)(base_vtx + 0),
		 };
		const uint32 start_idx = static_cast<uint32>(_snapshots[_writer_slot].idx_count);
		add_indices(idxs, 6);
		push_draw_call(0, start_idx, 6, base_vtx, static_cast<uint16>(sizeof(vertex_3d_line)));
	}

	void world_debug_rendering::draw_triangle(const vector3& p0, const vector3& p1, const vector3& p2, const color& col)
	{
		const vertex_simple v[3] = {
			{.pos = p0, .color = col.to_vector()},
			{.pos = p1, .color = col.to_vector()},
			{.pos = p2, .color = col.to_vector()},
		};
		const uint32		  base_vtx	= add_vertex_tri(v, 3);
		const primitive_index idxs[3]	= {(primitive_index)(base_vtx + 0), (primitive_index)(base_vtx + 1), (primitive_index)(base_vtx + 2)};
		const uint32		  start_idx = static_cast<uint32>(_snapshots[_writer_slot].idx_count);
		add_indices(idxs, 3);
		push_draw_call(1, start_idx, 3, base_vtx, static_cast<uint16>(sizeof(vertex_simple)));
	}

	void world_debug_rendering::draw_icon(const vector2&, const color& col)
	{
	}

	void world_debug_rendering::draw_text(const vector2&, const color& col)
	{
	}

	const world_debug_rendering::snapshot* world_debug_rendering::get_read_snapshot() const
	{
		uint32 idx = _published.load(std::memory_order_acquire);
		if (idx != UINT32_MAX)
			return &_snapshots[idx];
		return nullptr;
	}

}
