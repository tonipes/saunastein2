// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/vector.hpp"
#include "math/vector2ui16.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/semaphore_data.hpp"
#include "gfx/buffer.hpp"
#include "gfx/world/renderable.hpp"
#include "render_pass/render_pass_opaque.hpp"
#include "render_pass/render_pass_pre_depth.hpp"
#include "render_pass/render_pass_lighting.hpp"
#include "render_pass/render_pass_shadows.hpp"
#include "render_pass/render_pass_ssao.hpp"
#include "render_pass/render_pass_bloom.hpp"
#include "render_pass/render_pass_post_combiner.hpp"
#include "render_pass/render_pass_forward.hpp"
#include "view.hpp"

namespace SFG
{
	class texture_queue;
	class buffer_queue;
	class texture;
	class proxy_manager;

	class world_renderer
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
		world_renderer() = delete;
		world_renderer(proxy_manager& pm);

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(const vector2ui16& size, texture_queue* tq, buffer_queue* bq);
		void uninit();

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

		vector2ui16 _base_size = vector2ui16::zero;
	};
}
