// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "math/vector2ui16.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/gfx_common.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/render_pass.hpp"
#include "gfx/buffer.hpp"
#include "memory/bump_allocator.hpp"
#include "render_pass/render_pass_opaque.hpp"
#include "render_pass/render_pass_pre_depth.hpp"
#include "render_pass/render_pass_lighting.hpp"
#include "render_pass/render_pass_post_combiner.hpp"
#include "render_pass/render_pass_shadows.hpp"
#include "renderable_collector.hpp"

namespace SFG
{

	class texture_queue;
	class buffer_queue;
	class world;
	class texture;
	class proxy_manager;

#define MAX_BARRIERS 20

	class world_renderer
	{
	private:
		struct per_frame_data
		{
			gfx_id		   cmd_upload = 0;
			semaphore_data semp_frame = {};
			buffer		   entity_buffer;
			buffer		   bones_buffer;
			buffer		   point_lights_buffer;
			buffer		   dir_lights_buffer;
			buffer		   spot_lights_buffer;
		};

	public:
		world_renderer() = delete;
		world_renderer(proxy_manager& pm, world& w);

		void init(const vector2ui16& size, texture_queue* tq, buffer_queue* bq);
		void uninit();

		void prepare(uint8 frame_index);
		void render(uint8 frame_index, gfx_id layout_global, gfx_id bind_group_global, uint64 prev_copy, uint64 next_copy, gfx_id sem_copy);
		void resize(const vector2ui16& size);

		inline gfx_id get_output(uint8 frame_index)
		{
			return _pass_lighting.get_color_texture(frame_index);
		}

		inline const semaphore_data& get_final_semaphore(uint8 frame_index)
		{
			return _pfd[frame_index].semp_frame;
		}

	private:
		void upload(uint8 frame_index);

	private:
		proxy_manager& _proxy_manager;
		world&		   _world;

		texture_queue* _texture_queue = nullptr;
		buffer_queue*  _buffer_queue  = nullptr;

		renderable_collector	  _main_renderable_collector = {};
		render_pass_opaque		  _pass_opaque				 = {};
		render_pass_lighting	  _pass_lighting			 = {};
		render_pass_post_combiner _pass_post_combiner		 = {};
		render_pass_pre_depth	  _pass_pre_depth			 = {};
		render_pass_shadows		  _pass_shadows				 = {};

		per_frame_data _pfd[BACK_BUFFER_COUNT];
		vector2ui16	   _base_size			 = vector2ui16::zero;
		uint8*		   _shared_command_alloc = nullptr;
	};
}
