// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/semaphore_data.hpp"
#include "gfx/common/render_data.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/buffer.hpp"
#include "gfx/buffer_queue.hpp"
#include "gfx/texture_queue.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "memory/bump_allocator.hpp"

#ifndef SFG_PRODUCTION
#define USE_DEBUG_CONTROLLER
#include "app/debug_controller.hpp"
#endif

namespace SFG
{
	struct window_event;
	class vector2ui;
	class window;
	class render_data;
	class world_renderer;
	class world;
	class render_event_stream;

	class renderer
	{
	public:
		renderer(window& win, world& w, render_event_stream& event_stream);

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		bool init();
		void uninit();
		void wait_backend();
		void tick();
		void render();

		// -----------------------------------------------------------------------------
		// external control
		// -----------------------------------------------------------------------------

		bool		on_window_event(const window_event& ev);
		void		on_window_resize(const vector2ui16& size);
		void		on_swapchain_flags(uint8 flags);
		static void create_bind_layout_global();
		static void destroy_bind_layout_global();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline static gfx_id get_bind_layout_global()
		{
			return s_bind_layout_global;
		}

	private:
		struct buf_engine_global
		{
			float f1 = 0.0f;
			float f2 = 0.0;
		};

		struct per_frame_data
		{
			buffer		   buf_engine_global  = {};
			semaphore_data sem_frame		  = {};
			semaphore_data sem_copy			  = {};
			gfx_id		   cmd_gfx			  = 0;
			gfx_id		   cmd_copy			  = 0;
			gfx_id		   bind_group_global  = 0;
			uint32		   gpu_index_world_rt = 0;
#ifdef USE_DEBUG_CONTROLLER
			uint32 gpu_index_debug_controller_rt = 0;
#endif
		};

		struct gfx_data
		{
			gfx_id swapchain   = 0;
			uint8  frame_index = 0;
		};

		struct shader_data
		{
			gfx_id swapchain = 0;
		};

	private:
		world&				 _world;
		render_event_stream& _event_stream;
		window&				 _main_window;
		world_renderer*		 _world_renderer = nullptr;

#ifdef USE_DEBUG_CONTROLLER
		debug_controller _debug_controller = {};
#endif

		vector<barrier> _reuse_upload_barriers = {};
		gfx_data		_gfx_data			   = {};
		shader_data		_shaders			   = {};
		per_frame_data	_pfd[BACK_BUFFER_COUNT];
		bump_allocator	_frame_allocator[BACK_BUFFER_COUNT] = {};
		buffer_queue	_buffer_queue						= {};
		texture_queue	_texture_queue						= {};
		proxy_manager	_proxy_manager;
		vector2ui16		_base_size		 = {};
		uint8			_swapchain_flags = 0;

		static gfx_id s_bind_layout_global;
		static gfx_id s_bind_layout_global_compute;
		static gfx_id s_bind_group_global[BACK_BUFFER_COUNT];
	};
}
