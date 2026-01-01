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

#include "data/static_vector.hpp"

// gfx
#include "gfx/common/gfx_constants.hpp"
#include "gfx/buffer.hpp"
#include "gfx/common/texture_buffer.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/imgui_renderer.hpp"

// math
#include "math/vector2ui16.hpp"
#include "math/matrix4x4.hpp"
#include "math/vector4ui16.hpp"

// misc
#include "memory/bump_allocator.hpp"

// gui
#include "editor/gui/editor_gui_world_overlays.hpp"

namespace vekt
{
	struct draw_buffer;
	struct font;
	class builder;
	class atlas;
	class font_manager;
}

namespace SFG
{
	class texture_queue;
	class proxy_manager;
	class window;

	class editor_renderer
	{
	public:
		struct render_params
		{
			gfx_id			cmd_buffer;
			uint8			frame_index;
			bump_allocator& alloc;
			vector2ui16		size;
			gfx_id			global_layout;
			gfx_id			global_group;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(window& window, texture_queue* texture_queue);
		void uninit();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(proxy_manager& pm, gfx_id cmd_buffer, uint8 frame_index);
		void render(const render_params& p);
		void resize(const vector2ui16& size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gpu_index get_output_gpu_index(uint8 frame) const
		{
			return _pfd[frame].gpu_index_rt;
		}

		inline vekt::builder* get_builder() const
		{
			return _builder;
		}

	private:
		struct gui_draw_call
		{
			vector4ui16 scissors		= vector4ui16::zero;
			uint16		start_vtx		= 0;
			uint16		start_idx		= 0;
			uint16		index_count		= 0;
			gfx_id		shader			= 0;
			uint32		atlas_gpu_index = 0;
		};

		struct per_frame_data
		{
			buffer_cpu_gpu buf_gui_vtx	   = {};
			buffer_cpu_gpu buf_gui_idx	   = {};
			buffer_gpu	   buf_pass_data   = {};
			uint32		   counter_vtx	   = 0;
			uint32		   counter_idx	   = 0;
			gpu_index	   gpu_index_rt	   = NULL_GPU_INDEX;
			gfx_id		   hw_rt		   = NULL_GFX_ID;
			uint16		   draw_call_count = 0;

			inline void reset()
			{
				counter_vtx = counter_idx = 0;
				draw_call_count			  = 0;
			}
		};

		struct gui_pass_view
		{
			matrix4x4 proj			= matrix4x4::identity;
			float	  sdf_thickness = 0.5f;
			float	  sdf_softness	= 0.02f;
		};

		struct atlas_ref
		{
			vekt::atlas*   atlas			   = nullptr;
			gfx_id		   texture			   = 0;
			uint32		   texture_gpu_index   = 0;
			gfx_id		   intermediate_buffer = 0;
			texture_buffer buffer			   = {};

			inline bool operator==(const atlas_ref& other) const
			{
				return atlas == other.atlas && texture == other.texture && intermediate_buffer == other.intermediate_buffer;
			}
		};

		struct shaders
		{
			gfx_id gui_default = {};
			gfx_id gui_text	   = {};
			gfx_id gui_sdf	   = {};
		};

		struct gfx_data
		{
			static_vector<atlas_ref, 4> atlases;
			texture_queue*				texture_queue = nullptr;
			vector2ui16					screen_size	  = vector2ui16::zero;
			uint64						frame_counter = 0;
		};

	private:
		void create_textures(const vector2ui16& size);
		void destroy_textures();

		void		draw_vekt(uint8 frame_index, const vekt::draw_buffer& buffer);
		static void on_atlas_created(vekt::atlas* atlas, void* user_data);
		static void on_atlas_updated(vekt::atlas* atlas, void* user_data);
		static void on_atlas_destroyed(vekt::atlas* atlas, void* user_data);

	private:
		imgui_renderer		_imgui_renderer			= {};
		shaders				_shaders				= {};
		gfx_data			_gfx_data				= {};
		per_frame_data		_pfd[BACK_BUFFER_COUNT] = {};
		gui_draw_call		_gui_draw_calls[64]		= {};
		vekt::builder*		_builder				= nullptr;
		vekt::font_manager* _font_manager			= nullptr;
		vekt::font*			_font_main				= nullptr;

		// gui
		editor_gui_world_overlays _gui_world_overlays = {};
	};
}
