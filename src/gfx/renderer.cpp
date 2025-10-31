
// Copyright (c) 2025 Inan Evin

#include "renderer.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/world/world_renderer.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "platform/window.hpp"
#include "platform/time.hpp"
#include "math/vector4.hpp"
#include "memory/memory_tracer.hpp"
#include "io/log.hpp"
#include "common/system_info.hpp"
#include "resources/common_resources.hpp"
#include "engine_shaders.hpp"

namespace SFG
{
	gfx_id renderer::s_bind_group_global[BACK_BUFFER_COUNT] = {};
	gfx_id renderer::s_bind_layout_global					= 0;

	renderer::renderer(window& win, world& w, render_event_stream& event_stream) : _world(w), _main_window(win), _event_stream(event_stream), _proxy_manager(_buffer_queue, _texture_queue)
	{
		_reuse_upload_barriers.reserve(256);
	}

	void renderer::create_bind_layout_global()
	{
		s_bind_layout_global = gfx_util::create_bind_layout_global();
	}

	void renderer::destroy_bind_layout_global()
	{
		gfx_backend* backend = gfx_backend::get();
		backend->destroy_bind_layout(s_bind_layout_global);
		s_bind_layout_global = 0;
	}

	bool renderer::init()
	{
		gfx_backend* backend = gfx_backend::get();

		// swapchain
		_swapchain_flags = swapchain_flags::sf_vsync_every_v_blank;
		_base_size		 = _main_window.get_size();

		_gfx_data.swapchain = backend->create_swapchain({
			.window	   = _main_window.get_window_handle(),
			.os_handle = _main_window.get_platform_handle(),
			.scaling   = 1.0f,
			.format	   = render_target_definitions::get_format_swapchain(),
			.pos	   = vector2ui16::zero,
			.size	   = _base_size,
			.flags	   = _swapchain_flags,
		});

		// debug & world
#ifdef USE_DEBUG_CONTROLLER
		_debug_controller.init(&_texture_queue, s_bind_layout_global, _base_size);
#endif
		_world_renderer = new world_renderer(_proxy_manager);
		_world_renderer->init(_base_size, &_texture_queue, &_buffer_queue);

		// pfd
		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd		= _pfd[i];
			pfd.sem_frame.semaphore = backend->create_semaphore();
			pfd.sem_copy.semaphore	= backend->create_semaphore();
			pfd.cmd_gfx				= backend->create_command_buffer({
							.type		= command_type::graphics,
							.debug_name = "renderer_gfx",
			});

			pfd.cmd_copy = backend->create_command_buffer({
				.type		= command_type::transfer,
				.debug_name = "renderer_copy",
			});

			pfd.buf_engine_global.create_hw({.size = sizeof(buf_engine_global), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "engine_globals"});
			pfd.bind_group_global  = backend->create_empty_bind_group();
			pfd.gpu_index_world_rt = _world_renderer->get_output_gpu_index(i);

#ifdef USE_DEBUG_CONTROLLER
			pfd.gpu_index_debug_controller_rt = _debug_controller.get_output_gpu_index(i);
#endif

			backend->bind_group_add_descriptor(pfd.bind_group_global, 0, binding_type::ubo);
			backend->bind_group_update_descriptor(pfd.bind_group_global, 0, pfd.buf_engine_global.get_hw_gpu());

			_frame_allocator[i].init(1024 * 1024, 4);
			s_bind_group_global[i] = pfd.bind_group_global;
		}

		_buffer_queue.init();
		_texture_queue.init();
		_proxy_manager.init();

		_shaders.swapchain = engine_shaders::get().get_shader(engine_shader_type::engine_shader_type_renderer_swapchain).get_hw();

#ifdef SFG_TOOLMODE
		engine_shaders::get().add_reload_listener([this](engine_shader_type type, shader_direct& sh) {
			if (type == engine_shader_type::engine_shader_type_renderer_swapchain)
			{
				_shaders.swapchain = sh.get_hw();
				return;
			}
		});
#endif
		return true;
	}

	void renderer::uninit()
	{
		gfx_backend* backend = gfx_backend::get();

		// proxy
		_proxy_manager.fetch_render_events(_event_stream);
		_proxy_manager.uninit();

		// debug & world
		_world_renderer->uninit();
		delete _world_renderer;
#ifdef USE_DEBUG_CONTROLLER
		_debug_controller.uninit();
#endif

		// utils
		_texture_queue.uninit();
		_buffer_queue.uninit();

		// pfd
		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_semaphore(pfd.sem_frame.semaphore);
			backend->destroy_semaphore(pfd.sem_copy.semaphore);
			backend->destroy_command_buffer(pfd.cmd_gfx);
			backend->destroy_command_buffer(pfd.cmd_copy);
			backend->destroy_bind_group(pfd.bind_group_global);
			pfd.buf_engine_global.destroy();
			_frame_allocator[i].uninit();
		}

		// swp
		backend->destroy_swapchain(_gfx_data.swapchain);

		// globals
		backend->destroy_bind_layout(s_bind_layout_global);
		s_bind_layout_global = 0;
		engine_shaders::get().uninit();
	}

	void renderer::wait_backend()
	{
		SFG_INFO("renderer::wait_backend() - frame: {0}", frame_info::s_render_frame);

		gfx_backend* backend   = gfx_backend::get();
		const gfx_id queue_gfx = backend->get_queue_gfx();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->wait_semaphore(pfd.sem_frame.semaphore, pfd.sem_frame.value);
		}

		_proxy_manager.flush_destroys(true);
		_gfx_data.frame_index = backend->get_back_buffer_index(_gfx_data.swapchain);
	}

	void renderer::tick()
	{
#ifdef USE_DEBUG_CONTROLLER
		_debug_controller.tick();
#endif
	}

	void renderer::render()
	{
		gfx_backend*	  backend		 = gfx_backend::get();
		const gfx_id	  queue_gfx		 = backend->get_queue_gfx();
		const gfx_id	  queue_transfer = backend->get_queue_transfer();
		const vector2ui16 size			 = _base_size;

#ifndef SFG_PRODUCTION
		const int64 time_before_wait = time::get_cpu_microseconds();
#endif

		// Gate frame start to DXGI frame latency waitable for stable pacing
		backend->wait_for_swapchain_latency(_gfx_data.swapchain);

#ifndef SFG_PRODUCTION
		const int64 time_after_wait = time::get_cpu_microseconds();
		frame_info::s_render_present_microseconds.store(time_after_wait - time_before_wait);
#endif

		/* access frame data */
		const uint8	 frame_index   = _gfx_data.frame_index;
		const gfx_id layout_global = s_bind_layout_global;
		const gfx_id render_target = _gfx_data.swapchain;

		per_frame_data& pfd			   = _pfd[frame_index];
		const uint32	rt_world_index = pfd.gpu_index_world_rt;

#ifdef USE_DEBUG_CONTROLLER
		const uint32 rt_console_index = pfd.gpu_index_debug_controller_rt;
#endif

		bump_allocator& alloc = _frame_allocator[frame_index];
		alloc.reset();

		// Wait for frame's fence, then send any uploads needed.
		backend->wait_semaphore(pfd.sem_frame.semaphore, pfd.sem_frame.value);

		_proxy_manager.fetch_render_events(_event_stream);
		_proxy_manager.flush_destroys(false);

		/* access pfd */
		const gfx_id cmd_list		  = pfd.cmd_gfx;
		const gfx_id cmd_list_copy	  = pfd.cmd_copy;
		const gfx_id bg_global		  = pfd.bind_group_global;
		const gfx_id shader_swp		  = _shaders.swapchain;
		const gfx_id sem_frame		  = pfd.sem_frame.semaphore;
		const gfx_id sem_copy		  = pfd.sem_copy.semaphore;
		const uint64 prev_copy_value  = pfd.sem_copy.value;
		const uint64 next_frame_value = ++pfd.sem_frame.value;

		// Handle uploads
		if (!_buffer_queue.empty(frame_index) || !_texture_queue.empty())
		{
			pfd.sem_copy.value++;
			backend->reset_command_buffer(cmd_list_copy);
			_buffer_queue.flush_all(cmd_list_copy, frame_index, _reuse_upload_barriers);
			_texture_queue.flush_all(cmd_list_copy, _reuse_upload_barriers);
			backend->close_command_buffer(cmd_list_copy);
			backend->submit_commands(queue_transfer, &cmd_list_copy, 1);
			backend->queue_signal(queue_transfer, &sem_copy, &pfd.sem_copy.value, 1);
		}

		// Begin frame cmd list
		backend->reset_command_buffer(cmd_list);
		backend->cmd_bind_layout(cmd_list, {.layout = layout_global});
		backend->cmd_bind_group(cmd_list, {.group = bg_global});

		root_constants rc = {};
		backend->cmd_bind_constants(cmd_list,
									{
										.data		 = (uint8*)&rc,
										.offset		 = 0,
										.count		 = constant_index_max,
										.param_index = rpi_constants,
									});

		// Uploaded resources will have post-transitions.
		if (!_reuse_upload_barriers.empty())
		{
			backend->cmd_barrier(cmd_list,
								 {
									 .barriers		= _reuse_upload_barriers.data(),
									 .barrier_count = static_cast<uint16>(_reuse_upload_barriers.size()),
								 });
			_reuse_upload_barriers.resize(0);
		}

		const uint64 next_copy_value = pfd.sem_copy.value;

		const buf_engine_global globals = {};
		pfd.buf_engine_global.buffer_data(0, (void*)&globals, sizeof(buf_engine_global));

		// Handle debug controller
		{
			_debug_controller.prepare(frame_index);
			static_vector<barrier, 8> barriers;
			barriers.push_back({
				.resource	 = render_target,
				.flags		 = barrier_flags::baf_is_swapchain,
				.from_states = resource_state::resource_state_present,
				.to_states	 = resource_state::resource_state_render_target,
			});
			_debug_controller.collect_barriers(barriers);
			backend->cmd_barrier(cmd_list,
								 {
									 .barriers		= barriers.data(),
									 .barrier_count = static_cast<uint16>(barriers.size()),
								 });
			_debug_controller.render(cmd_list, frame_index, alloc);
		}

		_world_renderer->prepare(frame_index);
		_world_renderer->render(frame_index, layout_global, bg_global, prev_copy_value, next_copy_value, sem_copy);
		const semaphore_data& sem_world_data  = _world_renderer->get_final_semaphore(frame_index);
		const gfx_id		  sem_world		  = sem_world_data.semaphore;
		const uint64		  sem_world_value = sem_world_data.value;

		// swapchain pass
		{
			render_pass_color_attachment* attachment = alloc.allocate<render_pass_color_attachment>(1);
			attachment->clear_color					 = vector4(0.8f, 0.7f, 0.7f, 1.0f);
			attachment->load_op						 = load_op::clear;
			attachment->store_op					 = store_op::store;
			attachment->texture						 = render_target;

			BEGIN_DEBUG_EVENT(backend, cmd_list, "swapchain_pass");
			backend->cmd_begin_render_pass_swapchain(cmd_list, {.color_attachments = attachment, .color_attachment_count = 1});
			backend->cmd_set_scissors(cmd_list, {.width = static_cast<uint16>(size.x), .height = static_cast<uint16>(size.y)});
			backend->cmd_set_viewport(cmd_list, {.width = static_cast<uint16>(size.x), .height = static_cast<uint16>(size.y)});

			// gui pass bind group
			{
#ifdef USE_DEBUG_CONTROLLER
				const uint32 constants[2] = {rt_console_index, rt_world_index};
				const uint32 count		  = 2;
#else
				const uint32 constants[2] = {rt_world_index, 0};
				const uint32 count		  = 1;
#endif
				backend->cmd_bind_constants(cmd_list,
											{
												.data		 = (uint8*)&constants,
												.offset		 = constant_index_rp_constant0,
												.count		 = count,
												.param_index = rpi_constants,
											});
			}

			backend->cmd_bind_pipeline(cmd_list, {.pipeline = shader_swp});
			backend->cmd_draw_instanced(cmd_list, {.vertex_count_per_instance = 6, .instance_count = 1});
			backend->cmd_end_render_pass(cmd_list, {});
			END_DEBUG_EVENT(backend, cmd_list);
		}

		// Rt -> Present Barrier
		{
			static_vector<barrier, 1> barriers;

			barriers.push_back({
				.resource	 = render_target,
				.flags		 = barrier_flags::baf_is_swapchain,
				.from_states = resource_state::resource_state_render_target,
				.to_states	 = resource_state::resource_state_present,
			});

			backend->cmd_barrier(cmd_list,
								 {
									 .barriers		= barriers.data(),
									 .barrier_count = static_cast<uint16>(barriers.size()),
								 });
		}

		/*
			End the frame command list.
			Insert queue wait if there were any uploads.
			Submit & present, then insert queue signal for this frame's fence.
		*/
		backend->close_command_buffer(cmd_list);
		// backend->queue_wait(queue_gfx, &sem_world, &sem_world_value, 1);

		backend->submit_commands(queue_gfx, &cmd_list, 1);

		backend->present(&render_target, 1);
		_gfx_data.frame_index = backend->get_back_buffer_index(_gfx_data.swapchain);

		backend->queue_signal(queue_gfx, &sem_frame, &next_frame_value, 1);

		// SFG_TRACE("frame index {0}", (uint32)_gfx_data.frame_index);
#ifndef SFG_PRODUCTION
		const int64 time_end = time::get_cpu_microseconds();
		frame_info::s_render_work_microseconds.store(static_cast<double>(time_end - time_after_wait));
#endif
	}

	bool renderer::on_window_event(const window_event& ev)
	{
#ifdef USE_DEBUG_CONTROLLER
		return _debug_controller.on_window_event(ev);
#endif
		return false;
	}

	void renderer::on_window_resize(const vector2ui16& size)
	{
		SFG_VERIFY_THREAD_MAIN();
		_base_size = size;

		gfx_backend* backend = gfx_backend::get();

		backend->recreate_swapchain({
			.size	   = size,
			.swapchain = _gfx_data.swapchain,
			.flags	   = _swapchain_flags,
		});

		_world_renderer->resize(size);

#ifdef USE_DEBUG_CONTROLLER
		_debug_controller.on_window_resize(size);
#endif

		for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.gpu_index_world_rt = _world_renderer->get_output_gpu_index(i);
#ifdef USE_DEBUG_CONTROLLER
			pfd.gpu_index_debug_controller_rt = _debug_controller.get_output_gpu_index(i);
#endif
		}
	}

	void renderer::on_swapchain_flags(uint8 flags)
	{
		SFG_VERIFY_THREAD_MAIN();
		gfx_backend* backend = gfx_backend::get();
		_swapchain_flags	 = flags;
		backend->recreate_swapchain({
			.size	   = _base_size,
			.swapchain = _gfx_data.swapchain,
			.flags	   = _swapchain_flags,
		});
	}

}
