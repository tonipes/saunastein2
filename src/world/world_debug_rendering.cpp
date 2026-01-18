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
#include "math/math.hpp"
#include "math/vector3.hpp"
#include "math/vector2ui16.hpp"
#include "gfx/common/gfx_constants.hpp"

namespace SFG
{
	namespace
	{
		vector3 normalized_or_up(const vector3& v)
		{
			if (v.is_zero())
				return vector3::up;
			return v.normalized();
		}

		void make_basis(const vector3& direction, vector3& axis0, vector3& axis1, vector3& normal)
		{
			normal				 = normalized_or_up(direction);
			const float	  dot_up = math::abs(vector3::dot(normal, vector3::up));
			const vector3 ref	 = dot_up > 0.99f ? vector3::right : vector3::up;
			axis0				 = vector3::cross(ref, normal).normalized();
			axis1				 = vector3::cross(normal, axis0).normalized();
		}
	}

	void world_debug_rendering::init()
	{
		for (uint32 i = 0; i < 3; ++i)
		{
			snapshot& sh	 = _snapshots[i];
			sh.vertices_gui	 = new vertex_gui[MAX_VERTEX_COUNT_GUI];
			sh.vertices_line = new vertex_3d_line[MAX_VERTEX_COUNT_LINE];
			sh.vertices_tri	 = new vertex_simple[MAX_VERTEX_COUNT_TRI];
			sh.indices_gui	 = new primitive_index[MAX_INDEX_COUNT_GUI];
			sh.indices_line	 = new primitive_index[MAX_INDEX_COUNT_LINE];
			sh.indices_tri	 = new primitive_index[MAX_INDEX_COUNT_TRI];
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
			delete[] sh.indices_gui;
			delete[] sh.indices_tri;
			delete[] sh.indices_line;
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

	void world_debug_rendering::add_indices_line(const primitive_index* data, uint32 count)
	{
		snapshot&	 s		= _snapshots[_writer_slot];
		const uint32 before = s.idx_count_line;
		SFG_MEMCPY(&s.indices_line[before], data, sizeof(primitive_index) * count);
		s.idx_count_line += count;
	}

	void world_debug_rendering::add_indices_tri(const primitive_index* data, uint32 count)
	{
		snapshot&	 s		= _snapshots[_writer_slot];
		const uint32 before = s.idx_count_tri;
		SFG_MEMCPY(&s.indices_tri[before], data, sizeof(primitive_index) * count);
		s.idx_count_tri += count;
	}

	void world_debug_rendering::add_indices_gui(const primitive_index* data, uint32 count)
	{
		snapshot&	 s		= _snapshots[_writer_slot];
		const uint32 before = s.idx_count_gui;
		SFG_MEMCPY(&s.indices_gui[before], data, sizeof(primitive_index) * count);
		s.idx_count_gui += count;
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
		// snapshot& s = _snapshots[_writer_slot];
		//
		// const uint32 before = s.draw_call_count;
		//
		// debug_draw_call& dc = s.draw_calls[s.draw_call_count];
		// dc.start_index		= start_idx;
		// dc.index_count		= idx_count;
		// dc.base_vertex		= base_vtx;
		// dc.vertex_size		= vtx_size;
		// dc.vtx_buffer_index = vtx_buffer_index;
		// s.draw_call_count++;
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
		const uint32 start_idx = static_cast<uint32>(_snapshots[_writer_slot].idx_count_line);
		add_indices_line(idxs, 6);
		// push_draw_call(0, start_idx, 6, 0, static_cast<uint16>(sizeof(vertex_3d_line)));
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
		const uint32		  start_idx = static_cast<uint32>(_snapshots[_writer_slot].idx_count_tri);
		add_indices_tri(idxs, 3);
		// push_draw_call(1, start_idx, 3, base_vtx, static_cast<uint16>(sizeof(vertex_simple)));
	}

	void world_debug_rendering::draw_box(const vector3& center, const vector3& half_extents, const color& col, float thickness)
	{
		// 8 corners (axis-aligned)
		const vector3 he = half_extents;

		const vector3 c[8] = {
			center + vector3(-he.x, -he.y, -he.z), // 0
			center + vector3(+he.x, -he.y, -he.z), // 1
			center + vector3(+he.x, -he.y, +he.z), // 2
			center + vector3(-he.x, -he.y, +he.z), // 3
			center + vector3(-he.x, +he.y, -he.z), // 4
			center + vector3(+he.x, +he.y, -he.z), // 5
			center + vector3(+he.x, +he.y, +he.z), // 6
			center + vector3(-he.x, +he.y, +he.z), // 7
		};

		// bottom rectangle
		draw_line(c[0], c[1], col, thickness);
		draw_line(c[1], c[2], col, thickness);
		draw_line(c[2], c[3], col, thickness);
		draw_line(c[3], c[0], col, thickness);

		// top rectangle
		draw_line(c[4], c[5], col, thickness);
		draw_line(c[5], c[6], col, thickness);
		draw_line(c[6], c[7], col, thickness);
		draw_line(c[7], c[4], col, thickness);

		// vertical edges
		draw_line(c[0], c[4], col, thickness);
		draw_line(c[1], c[5], col, thickness);
		draw_line(c[2], c[6], col, thickness);
		draw_line(c[3], c[7], col, thickness);
	}

	void world_debug_rendering::draw_oriented_circle(const vector3& center, float radius, const vector3& direction, const color& col, float thickness, uint32 segments)
	{
		if (segments < 3)
			segments = 3;

		vector3 axis0, axis1, normal;
		make_basis(direction, axis0, axis1, normal);

		const float two_pi = 2.0f * MATH_PI;

		for (uint32 i = 0; i < segments; ++i)
		{
			const float t0 = (float)i / (float)segments;
			const float t1 = (float)(i + 1) / (float)segments;

			const float a0 = two_pi * t0;
			const float a1 = two_pi * t1;

			const float c0 = math::cos(a0);
			const float s0 = math::sin(a0);
			const float c1 = math::cos(a1);
			const float s1 = math::sin(a1);

			const vector3 p0 = center + (axis0 * (c0 * radius)) + (axis1 * (s0 * radius));
			const vector3 p1 = center + (axis0 * (c1 * radius)) + (axis1 * (s1 * radius));

			draw_line(p0, p1, col, thickness);
		}
	}

	void world_debug_rendering::draw_sphere(const vector3& center, float radius, const color& col, float thickness, uint32 segments)
	{
		draw_oriented_circle(center, radius, vector3::up, col, thickness, segments);
		draw_oriented_circle(center, radius, vector3::right, col, thickness, segments);
		draw_oriented_circle(center, radius, vector3::forward, col, thickness, segments);
	}

	void world_debug_rendering::draw_oriented_hemisphere(const vector3& center, float radius, const vector3& direction, const color& col, float thickness, uint32 segments)
	{
		if (segments < 4)
			segments = 4;

		vector3 axis0, axis1, normal;
		make_basis(direction, axis0, axis1, normal);

		draw_oriented_circle(center, radius, normal, col, thickness, segments);

		const float two_pi = 2.0f * MATH_PI;

		auto draw_meridian = [&](const vector3& lateral_axis) {
			vector3 prev = center + (lateral_axis * radius);
			for (uint32 i = 1; i <= segments; ++i)
			{
				const float t = (float)i / (float)segments;
				const float a = MATH_PI * t;

				const float ca = math::cos(a);
				const float sa = math::sin(a);

				const vector3 p = center + (lateral_axis * (ca * radius)) + (normal * (sa * radius));
				draw_line(prev, p, col, thickness);
				prev = p;
			}
		};

		draw_meridian(axis0);
		draw_meridian(axis1);
		draw_meridian((axis0 + axis1).normalized());
		draw_meridian((axis0 - axis1).normalized());
	}

	void world_debug_rendering::draw_capsule(const vector3& center, float radius, float half_height, const color& col, float thickness, uint32 segments)
	{
		if (segments < 3)
			segments = 3;

		const vector3 up		 = vector3::up;
		const vector3 top_center = center + up * half_height;
		const vector3 bot_center = center - up * half_height;

		vector3 axis0, axis1, normal;
		make_basis(up, axis0, axis1, normal);

		const float two_pi = 2.0f * MATH_PI;
		for(float t = 0.0f; t < two_pi; t += MATH_PI * 0.25f)
		{
			const float ca = math::cos(t);
			const float sa = math::sin(t);

			const vector3 ring_offset = (axis0 * (ca * radius)) + (axis1 * (sa * radius));
			const vector3 p_top		  = top_center + ring_offset;
			const vector3 p_bot		  = bot_center + ring_offset;

			draw_line(p_top, p_bot, col, thickness);
		}

		draw_oriented_hemisphere(top_center, radius, up, col, thickness, segments);
		draw_oriented_hemisphere(bot_center, radius, -up, col, thickness, segments);
	}

	void world_debug_rendering::draw_oriented_cone(const vector3& apex, const vector3& direction, float length, float radius, const color& col, float thickness, uint32 segments)
	{

		const vector3 dir		  = normalized_or_up(direction);
		const vector3 base_center = apex + dir * length;

		vector3 axis0, axis1, normal;
		make_basis(dir, axis0, axis1, normal);

		draw_oriented_circle(base_center, radius, dir, col, thickness, segments);

		const float two_pi = 2.0f * MATH_PI;
		for (uint32 i = 0; i < segments; ++i)
		{
			const float t = (float)i / (float)segments;
			const float a = two_pi * t;

			const float ca = math::cos(a);
			const float sa = math::sin(a);

			const vector3 p = base_center + (axis0 * (ca * radius)) + (axis1 * (sa * radius));
			draw_line(apex, p, col, thickness);
		}

		draw_line(base_center + axis0 * radius, base_center - axis0 * radius, col, thickness);
		draw_line(base_center + axis1 * radius, base_center - axis1 * radius, col, thickness);
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
