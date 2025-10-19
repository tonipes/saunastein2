// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/gfx_common.hpp"
#include "gfx/common/render_data.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/buffer.hpp"
#include "gfx/buffer_queue.hpp"
#include "gfx/texture_queue.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "resources/shader_direct.hpp"
#include "memory/bump_allocator.hpp"
#include "app/debug_controller.hpp"

namespace SFG
{
	class vector2ui;
	class window;
	class render_data;
	class world_renderer;
	class world;

	struct window_event;

#ifndef SFG_PRODUCTION
#define USE_DEBUG_CONTROLLER
#endif

	class render_event_stream;

	class renderer
	{
	public:
		renderer();

		bool init(window* main_window, world* world);
		void uninit(render_event_stream& stream);
		void wait_backend();
		void tick();
		void render(render_event_stream& stream, const vector2ui16& size);
		bool on_window_event(const window_event& ev);
		void on_window_resize(const vector2ui16& size);
		void on_swapchain_flags(uint8 flags);

		inline texture_queue& get_texture_queue()
		{
			return _texture_queue;
		}

		inline buffer_queue& get_buffer_queue()
		{
			return _buffer_queue;
		}

		inline world_renderer* get_world_renderer() const
		{
			return _world_renderer;
		}

		inline static gfx_id get_bind_layout_global()
		{
			return s_bind_layout_global;
		}

		inline static gfx_id get_bind_group_global(uint8 index)
		{
			return s_bind_group_global[index];
		}

	private:
	private:
		struct buf_engine_global
		{
			float f1 = 0.0f;
			float f2 = 0.0;
		};

		struct per_frame_data
		{
			buffer		   buf_engine_global	= {};
			semaphore_data sem_frame			= {};
			semaphore_data sem_copy				= {};
			gfx_id		   cmd_gfx				= 0;
			gfx_id		   cmd_copy				= 0;
			gfx_id		   bind_group_global	= 0;
			gfx_id		   bind_group_swapchain = 0;
		};

		struct gfx_data
		{
			gfx_id swapchain		   = 0;
			gfx_id bind_layout_global  = 0;
			gfx_id dummy_ubo		   = 0;
			gfx_id dummy_ssbo		   = 0;
			gfx_id dummy_sampler	   = 0;
			gfx_id dummy_texture	   = 0;
			gfx_id dummy_texture_array = 0;
			gfx_id dummy_texture_cube  = 0;
			uint8  frame_index		   = 0;
		};

		struct shader_data
		{
			gfx_id swapchain = 0;
		};

	private:
		world_renderer* _world_renderer = nullptr;

#ifdef USE_DEBUG_CONTROLLER
		debug_controller _debug_controller = {};
#endif
		world*			_world	  = nullptr;
		gfx_data		_gfx_data = {};
		shader_data		_shaders  = {};
		per_frame_data	_pfd[BACK_BUFFER_COUNT];
		bump_allocator	_frame_allocator[BACK_BUFFER_COUNT] = {};
		buffer_queue	_buffer_queue						= {};
		texture_queue	_texture_queue						= {};
		proxy_manager	_proxy_manager;
		vector<barrier> _reuse_upload_barriers = {};
		vector2ui16		_base_size			   = {};
		uint8			_swapchain_flags	   = 0;

		static gfx_id s_bind_layout_global;
		static gfx_id s_bind_group_global[BACK_BUFFER_COUNT];
	};
}
