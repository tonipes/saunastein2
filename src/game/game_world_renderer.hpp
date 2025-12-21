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

#include "common/size_definitions.hpp"
#include "data/vector.hpp"
#include "math/vector2ui16.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/semaphore_data.hpp"
#include "gfx/buffer.hpp"
#include "gfx/world/renderable.hpp"
#include "gfx/world/render_pass/render_pass_opaque.hpp"
#include "gfx/world/render_pass/render_pass_pre_depth.hpp"
#include "gfx/world/render_pass/render_pass_lighting.hpp"
#include "gfx/world/render_pass/render_pass_shadows.hpp"
#include "gfx/world/render_pass/render_pass_ssao.hpp"
#include "gfx/world/render_pass/render_pass_bloom.hpp"
#include "gfx/world/render_pass/render_pass_post_combiner.hpp"
#include "gfx/world/render_pass/render_pass_forward.hpp"
#include "gfx/world/render_pass/render_pass_canvas_2d.hpp"

#ifdef SFG_TOOLMODE
#include "gfx/world/render_pass/render_pass_object_id.hpp"
#include "gfx/world/render_pass/render_pass_selection_outline.hpp"
#endif

#ifdef JPH_DEBUG_RENDERER
#include "gfx/world/render_pass/render_pass_physics_debug.hpp"
#endif

#include "gfx/world/view.hpp"

namespace SFG
{
	class texture_queue;
	class buffer_queue;
	class texture;
	class proxy_manager;
	class world;

	class game_world_renderer
	{
	private:
		struct per_frame_data
		{
			buffer		   shadow_data_buffer;
			buffer		   entity_buffer;
			buffer		   bones_buffer;
			buffer		   point_lights_buffer;
			buffer		   dir_lights_buffer;
			buffer		   spot_lights_buffer;
			buffer		   float_buffer;
			semaphore_data semp_frame		   = {};
			semaphore_data semp_ssao		   = {};
			semaphore_data semp_lighting	   = {};
			gfx_id		   cmd_upload		   = NULL_GFX_ID;
			uint32		   _float_buffer_count = 0;

			inline void reset()
			{
				_float_buffer_count = 0;
			}
		};

	public:
		game_world_renderer() = delete;
		game_world_renderer(proxy_manager& pm, world& w);

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(const vector2ui16& size, texture_queue* tq, buffer_queue* bq);
		void uninit();
		void tick();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void   prepare(uint8 frame_index);
		void   render(uint8 frame_index, gfx_id layout_global, gfx_id layout_global_compute, gfx_id bind_group_global, uint64 prev_copy, uint64 next_copy, gfx_id sem_copy);
		void   resize(const vector2ui16& size);
		uint32 add_to_float_buffer(uint8 frame_index, float f);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gpu_index get_output_gpu_index(uint8 frame_index)
		{
			return _pass_post.get_output_gpu_index(frame_index);
		}

		inline const semaphore_data& get_final_semaphore(uint8 frame_index)
		{
			return _pfd[frame_index].semp_frame;
		}

#ifdef SFG_TOOLMODE

		inline render_pass_object_id& get_render_pass_object_id()
		{
			return _pass_object_id;
		}

		inline render_pass_selection_outline& get_render_pass_selection_outline()
		{
			return _pass_selection_outline;
		}

#endif

	private:
		// -----------------------------------------------------------------------------
		// world collect
		// -----------------------------------------------------------------------------

		void collect_and_upload(uint8 frame_index);
		void collect_and_upload_entities(gfx_id cmd_buffer, uint8 frame_index);
		void collect_and_upload_bones(gfx_id cmd_buffer, uint8 frame_index);
		void collect_and_upload_lights(gfx_id cmd_buffer, uint8 frame_index);

	private:
		proxy_manager&			  _proxy_manager;
		world&					  _world;
		per_frame_data			  _pfd[BACK_BUFFER_COUNT];
		view					  _main_camera_view = {};
		vector<renderable_object> _renderables;

		render_pass_pre_depth	  _pass_pre_depth = {};
		render_pass_opaque		  _pass_opaque	  = {};
		render_pass_lighting	  _pass_lighting  = {};
		render_pass_shadows		  _pass_shadows	  = {};
		render_pass_ssao		  _pass_ssao	  = {};
		render_pass_bloom		  _pass_bloom	  = {};
		render_pass_post_combiner _pass_post	  = {};
		render_pass_forward		  _pass_forward	  = {};
		render_pass_canvas_2d	  _pass_canvas_2d = {};

#ifdef JPH_DEBUG_RENDERER
		render_pass_physics_debug _pass_physics_debug = {};
#endif

#ifdef SFG_TOOLMODE
		render_pass_object_id		  _pass_object_id		  = {};
		render_pass_selection_outline _pass_selection_outline = {};
		uint32						  _target_world_id		  = 0;
#endif

		vector2ui16 _base_size = vector2ui16::zero;
	};
}
