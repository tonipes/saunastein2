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

#pragma once

#include "data/atomic.hpp"
#include "data/vector.hpp"
#include "resources/vertex.hpp"
#include "gfx/common/gfx_constants.hpp"

namespace vekt
{
	class builder;
}

namespace SFG
{

	class vector2;
	class vector3;
	class color;

	struct debug_draw_call
	{
		uint32 start_index		= 0;
		uint32 index_count		= 0;
		uint32 base_vertex		= 0;
		uint16 vertex_size		= 0;
		uint32 vtx_buffer_index = 0; // line, tri, gui default, gui text, gui sdf
	};

	class world_debug_rendering
	{
	public:
		struct snapshot
		{
			vertex_simple*	 vertices_tri	= nullptr;
			vertex_3d_line*	 vertices_line	= nullptr;
			vertex_gui*		 vertices_gui	= nullptr;
			primitive_index* indices_line	= nullptr;
			primitive_index* indices_tri	= nullptr;
			primitive_index* indices_gui	= nullptr;
			uint32			 vtx_count_line = 0;
			uint32			 vtx_count_tri	= 0;
			uint32			 vtx_count_gui	= 0;
			uint32			 idx_count_line = 0;
			uint32			 idx_count_tri	= 0;
			uint32			 idx_count_gui	= 0;

			void reset()
			{
				vtx_count_line = 0;
				vtx_count_tri  = 0;
				vtx_count_gui  = 0;
				idx_count_line = 0;
				idx_count_tri  = 0;
				idx_count_gui  = 0;
			}
		};

		static constexpr size_t MAX_VERTEX_COUNT_LINE = 32000;
		static constexpr size_t MAX_VERTEX_COUNT_TRI  = 32000;
		static constexpr size_t MAX_VERTEX_COUNT_GUI  = 32000;
		static constexpr size_t MAX_INDEX_COUNT_TRI	  = 96000;
		static constexpr size_t MAX_INDEX_COUNT_LINE  = 96000;
		static constexpr size_t MAX_INDEX_COUNT_GUI	  = 96000;
		static constexpr size_t MAX_DRAW_CALLS		  = 4096;

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void begin_frame(const vector2ui16& res);
		void end_frame();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void			draw_line(const vector3& p0, const vector3& p1, const color& col, float thickness);
		void			draw_triangle(const vector3& p0, const vector3& p1, const vector3& p2, const color& col);
		void			draw_box(const vector3& center, const vector3& half_extents, const vector3& orientation, const color& col, float thickness);
		void			draw_capsule(const vector3& center, float radius, float half_height, const vector3& direction, const color& col, float thickness, uint32 segments = 64);
		void			draw_cylinder(const vector3& center, float radius, float half_height, const vector3& direction, const color& col, float thickness, uint32 segments = 64);
		void			draw_sphere(const vector3& center, float radius, const color& col, float thickness, uint32 segments = 64);
		void			draw_oriented_hemisphere(const vector3& center, float radius, const vector3& direction, const color& col, float thickness, uint32 segments = 64);
		void			draw_oriented_circle(const vector3& center, float radius, const vector3& direction, const color& col, float thickness, uint32 segments = 64);
		void			draw_oriented_cone(const vector3& apex, const vector3& direction, float length, float radius, const color& col, float thickness, uint32 segments = 64);
		void			draw_oriented_plane(const vector3& center, float width, float height, const vector3& orientation, const color& col, float thickness, uint32 segments = 8);
		void			draw_frustum(const vector3& origin, const vector3& direction, float fov_degrees, float aspect_ratio, float near_distance, float far_distance, const color& col, float thickness);
		void			draw_icon(const vector2& pos, const color& col);
		void			draw_text(const vector2& pos, const color& col);
		const snapshot* get_read_snapshot() const;

	private:
		void   add_indices_line(const primitive_index* data, uint32 count);
		void   add_indices_tri(const primitive_index* data, uint32 count);
		void   add_indices_gui(const primitive_index* data, uint32 count);
		uint32 add_vertex_line(const vertex_3d_line* data, uint32 count);
		uint32 add_vertex_tri(const vertex_simple* data, uint32 count);
		uint32 add_vertex_gui(const vertex_gui* data, uint32 count);

		void push_draw_call(uint32 vtx_buffer_index, uint32 start_idx, uint32 idx_count, uint32 base_vtx, uint16 vtx_size);

	private:
		vekt::builder* _builder		 = nullptr;
		snapshot	   _snapshots[3] = {};
		atomic<uint32> _published	 = UINT32_MAX;
		atomic<uint32> _reader_slot	 = UINT32_MAX;
		uint32		   _writer_slot	 = 0;
		uint32		   _current_read = UINT32_MAX;
	};
}
