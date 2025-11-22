// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/vector.hpp"
#include "math/vector2ui16.hpp"

#include "gfx/buffer.hpp"
#include "gfx/world/draws.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/world/view.hpp"
#include "gfx/world/renderable.hpp"
#include "gfx/draw_stream.hpp"
#include "gfx/common/barrier_description.hpp"

#include "game/game_max_defines.hpp"
#include "memory/bump_allocator.hpp"
#include "math/vector4.hpp"
#include "math/matrix4x4.hpp"

namespace SFG
{
	struct view;
	struct vector2ui16;
	class proxy_manager;
	struct view;
	struct world_render_data;

	class render_pass_shadows
	{
	private:
		struct ubo
		{
			matrix4x4 view_proj = matrix4x4::identity;
		};

		struct per_frame_data
		{
			gfx_id cmd_buffer = NULL_GFX_ID;
		};

		struct pass
		{
			draw_stream				  stream				  = {};
			vector<renderable_object> renderables			  = {};
			view					  pass_view				  = {};
			buffer					  ubos[BACK_BUFFER_COUNT] = {};
			vector2ui16				  resolution			  = vector2ui16::zero;
			gfx_id					  texture				  = {};
			uint8					  view_index			  = 0;
			uint8					  transition_owner		  = 0;
		};

	public:
		struct render_params
		{
			uint8			   frame_index;
			const vector2ui16& size;
			gpu_index		   gpu_index_entities;
			gpu_index		   gpu_index_bones;
			gfx_id			   global_layout;
			gfx_id			   global_group;
		};

		struct pass_props
		{
			proxy_manager&	   pm;
			uint8			   frame_index;
			const vector2ui16& res;
			gfx_id			   texture;
			uint8			   transition_owner;
			uint8			   view_index;
			const matrix4x4&   proj;
			const matrix4x4&   view;
			const vector3&	   position;
			float			   cascade_near;
			float			   cascade_far;
			float			   fov;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(proxy_manager& pm, const view& main_camera_view, const vector2ui16& resolution, uint8 frame_index);
		void render(const render_params& params);
		void add_pass(const pass_props& p);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline gfx_id get_cmd_buffer(uint8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

		inline uint16		get_pass_count() const
		{
			return _pass_count;
		}
	private:
		pass*			_passes		= nullptr;
		uint16			_pass_count = 0;
		per_frame_data	_pfd[BACK_BUFFER_COUNT];
		vector<barrier> _barriers;
		bump_allocator	_alloc = {};
	};
}
