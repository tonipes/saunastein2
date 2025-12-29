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

#include "gfx/buffer.hpp"
#include "gfx/common/gfx_constants.hpp"

#include "math/matrix4x4.hpp"
#include "math/vector2.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	struct view;
	struct renderable_object;
	class physics_debug_renderer;
	class world;
	class proxy_manager;
	class physics_world;

	class render_pass_physics_debug
	{
	private:
		struct ubo
		{
			matrix4x4 view					= matrix4x4::identity;
			matrix4x4 proj					= matrix4x4::identity;
			matrix4x4 view_proj				= matrix4x4::identity;
			vector4	  resolution_and_planes = vector4::zero;
		};

		struct per_frame_data
		{
			buffer		   ubo;
			buffer_cpu_gpu triangle_vertices;
			buffer_cpu_gpu line_vertices;
			buffer_cpu_gpu triangle_indices;
			buffer_cpu_gpu line_indices;
			gfx_id		   cmd_buffer		   = 0;
			uint32		   _triangle_idx_count = 0;
			uint32		   _line_idx_count	   = 0;
		};

	public:
		struct render_params
		{
			uint8			   frame_index;
			const vector2ui16& size;
			gfx_id			   depth_texture;
			gfx_id			   input_texture;
			gfx_id			   global_layout;
			gfx_id			   global_group;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(const vector2ui16& sizes);
		void uninit();
		void tick(world& w);

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(const view& main_camera_view, const vector2ui16& resolution, uint8 frame_index);
		void render(const render_params& params);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gfx_id get_cmd_buffer(uint8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

	private:
		per_frame_data			_pfd[BACK_BUFFER_COUNT];
		gfx_id					_shader_debug_default = {};
		gfx_id					_shader_debug_line	  = {};
		physics_debug_renderer* _renderer			  = nullptr;
	};
}
