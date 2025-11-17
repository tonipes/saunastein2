// Copyright (c) 2025 Inan Evin
#pragma once

#include "data/static_vector.hpp"

#include "gfx/common/gfx_constants.hpp"
#include "gfx/buffer.hpp"
#include "gfx/common/texture_buffer.hpp"
#include "gfx/common/barrier_description.hpp"
#include "memory/bump_allocator.hpp"
#include "math/vector2ui16.hpp"
#include "math/matrix4x4.hpp"
#include "math/vector4ui16.hpp"

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

	class editor_gui_renderer
	{
	public:
		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(texture_queue* texture_queue, const vector2ui16& screen_size);
		void uninit();

		// -----------------------------------------------------------------------------
		// rendering
		// -----------------------------------------------------------------------------

		void prepare(gfx_id cmd_buffer, uint8 frame_index);
		void render_in_swapchain(gfx_id cmd_buffer, uint8 frame_index, bump_allocator& alloc);
		void resize(const vector2ui16& size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

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
			buffer buf_gui_vtx		 = {};
			buffer buf_gui_idx		 = {};
			buffer buf_gui_pass_view = {};
			uint32 counter_vtx		 = 0;
			uint32 counter_idx		 = 0;
			uint16 draw_call_count	 = 0;

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
			vector2ui16					window_size	  = vector2ui16::zero;
			uint64						frame_counter = 0;
			uint8						frame_index	  = 0;
		};

	private:
		void		on_draw(const vekt::draw_buffer& buffer);
		static void on_atlas_created(vekt::atlas* atlas, void* user_data);
		static void on_atlas_updated(vekt::atlas* atlas, void* user_data);
		static void on_atlas_destroyed(vekt::atlas* atlas, void* user_data);

	private:
		shaders				_shaders				= {};
		gfx_data			_gfx_data				= {};
		per_frame_data		_pfd[BACK_BUFFER_COUNT] = {};
		gui_draw_call		_gui_draw_calls[64]		= {};
		vekt::builder*		_builder				= nullptr;
		vekt::font_manager* _font_manager			= nullptr;
		vekt::font*			_font_main				= nullptr;
	};
}
