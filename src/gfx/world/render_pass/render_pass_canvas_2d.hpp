// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/vector.hpp"
#include "data/static_vector.hpp"

#include "gfx/buffer.hpp"
#include "gfx/world/draws.hpp"
#include "gfx/draw_stream.hpp"
#include "gfx/common/gfx_constants.hpp"

#include "memory/bump_allocator.hpp"
#include "math/matrix4x4.hpp"
#include "math/vector4.hpp"
#include "math/vector2.hpp"
#include "game/game_max_defines.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	struct view;
	struct renderable_object;
	class proxy_manager;

	class render_pass_canvas_2d
	{
	private:
		struct ubo
		{
			matrix4x4 proj		 = matrix4x4::identity;
			vector2	  resolution = vector2::zero;
		};

		struct per_frame_data
		{
			buffer ubo;
			gfx_id cmd_buffer = 0;
		};

	public:
		struct render_params
		{
			uint8			   frame_index;
			const vector2ui16& size;
			gfx_id			   input_texture;
			gfx_id			   global_layout;
			gfx_id			   global_group;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(const vector2ui16& sizes);
		void uninit();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(proxy_manager& pm, const vector<renderable_object>& renderables, const view& main_camera_view, const vector2ui16& resolution, uint8 frame_index);
		void render(const render_params& params);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gfx_id get_cmd_buffer(uint8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

	private:
		per_frame_data	_pfd[BACK_BUFFER_COUNT];
		draw_stream_gui _draw_stream;
		bump_allocator	_alloc = {};
	};
}
