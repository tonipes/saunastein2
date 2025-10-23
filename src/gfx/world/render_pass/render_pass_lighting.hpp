// Copyright (c) 2025 Inan Evin

#pragma once
#include "data/static_vector.hpp"
#include "gfx/buffer.hpp"
#include "gfx/world/draws.hpp"
#include "memory/bump_allocator.hpp"
#include "math/vector3.hpp"

namespace SFG
{
	struct view;
	class proxy_manager;
	struct vector2ui16;
	struct view;
	struct world_render_data;

	class render_pass_lighting
	{
	private:
		struct ubo
		{
			vector3 ambient_color	   = vector3::one;
			uint32	point_lights_count = 0;
			uint32	spot_lights_count  = 0;
			uint32	dir_lights_count   = 0;
			uint32	padding[2]		   = {};
		};

		struct per_frame_data
		{
			buffer		   ubo;
			semaphore_data semaphore;
			gfx_id		   cmd_buffer;
			gfx_id		   bind_group;
			gfx_id		   render_target = 0;
		};

	public:
		struct init_params
		{
			const vector2ui16& size;
			uint8*			   alloc;
			size_t			   alloc_size;
			gfx_id*			   entities;
			gfx_id*			   point_lights;
			gfx_id*			   spot_lights;
			gfx_id*			   dir_lights;
			gfx_id*			   gbuffer_textures;
		};

		struct render_params
		{
			uint8			   frame_index;
			const vector2ui16& size;
			gfx_id			   global_layout;
			gfx_id			   global_group;
		};
		void init(const init_params& params);
		void uninit();

		void prepare(proxy_manager& pm, view& camera_view, uint8 frame_index);
		void render(const render_params& params);
		void resize(const vector2ui16& size, gfx_id* gbuffer_textures);

		inline gfx_id get_color_texture(uint8 frame_index) const
		{
			return _pfd[frame_index].render_target;
		}

		inline semaphore_data& get_semaphore(uint8 frame_index)
		{
			return _pfd[frame_index].semaphore;
		}

		inline gfx_id get_cmd_buffer(uint8 frame_index) const
		{
			return _pfd[frame_index].cmd_buffer;
		}

	private:
		void destroy_textures();
		void create_textures(const vector2ui16& sz, gfx_id* gbuffer_textures);

	private:
		per_frame_data _pfd[BACK_BUFFER_COUNT];
		gfx_id		   _shader_lighting = 0;
		bump_allocator _alloc			= {};
	};
}
