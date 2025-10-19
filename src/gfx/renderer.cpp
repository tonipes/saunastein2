
// Copyright (c) 2025 Inan Evin

#include "renderer.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/world/world_renderer.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "platform/window.hpp"
#include "platform/time.hpp"
#include "math/vector4.hpp"
#include "memory/memory_tracer.hpp"
#include "io/log.hpp"
#include "common/system_info.hpp"
#include "world/world.hpp"
#include "resources/shader_raw.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{
#define RT_FORMAT format::r8g8b8a8_srgb

	gfx_id renderer::s_bind_group_global[BACK_BUFFER_COUNT] = {};
	gfx_id renderer::s_bind_layout_global					= 0;

	renderer::renderer() : _proxy_manager(_buffer_queue, _texture_queue)
	{
		_reuse_upload_barriers.reserve(32);
	}

	void renderer::init(window* main_window, world* w)
	{
		_world				 = w;
		gfx_backend* backend = gfx_backend::get();

		_swapchain_flags = swapchain_flags::sf_vsync_every_v_blank;

		_gfx_data.swapchain = backend->create_swapchain({
			.window	   = main_window->get_window_handle(),
			.os_handle = main_window->get_platform_handle(),
			.scaling   = 1.0f,
			.format	   = RT_FORMAT,
			.pos	   = vector2ui16::zero,
			.size	   = main_window->get_size(),
			.flags	   = _swapchain_flags,
		});
		_base_size			= main_window->get_size();

		_gfx_data.bind_layout_global = gfx_util::create_bind_layout_global();
		s_bind_layout_global		 = _gfx_data.bind_layout_global;

		_gfx_data.dummy_sampler = backend->create_sampler({});
		_gfx_data.dummy_texture = backend->create_texture({
			.texture_format = format::r8_unorm,
			.size			= vector2ui16(1, 1),
			.flags			= texture_flags::tf_is_2d | texture_flags::tf_sampled,
		});

		_gfx_data.dummy_texture_array = backend->create_texture({
			.texture_format = format::r8_unorm,
			.size			= vector2ui16(1, 1),
			.flags			= texture_flags::tf_is_2d | texture_flags::tf_sampled,
			.array_length	= 2,
		});

		_gfx_data.dummy_texture_cube = backend->create_texture({
			.texture_format = format::r8_unorm,
			.size			= vector2ui16(1, 1),
			.flags			= texture_flags::tf_is_2d | texture_flags::tf_sampled | tf_cubemap,
		});
		_gfx_data.dummy_ubo			 = backend->create_resource({.size = 4, .flags = resource_flags::rf_constant_buffer | resource_flags::rf_gpu_only});
		_gfx_data.dummy_ssbo		 = backend->create_resource({.size = 4, .flags = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_only});

		shader_raw raw = {};
		raw.cook_from_file("assets/engine/shaders/swapchain/swapchain.stkfrg", false, _gfx_data.bind_layout_global, false);
		_shaders.swapchain.create_from_raw(raw);
		raw.destroy();

#ifdef USE_DEBUG_CONTROLLER
		_debug_controller.init(&_texture_queue, _gfx_data.bind_layout_global, main_window->get_size());
#endif
		_world_renderer = new world_renderer(_proxy_manager, *w);
		_world_renderer->init(main_window->get_size(), &_texture_queue, &_buffer_queue);
		w->set_world_renderer(_world_renderer);

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
			pfd.bind_group_global = gfx_util::create_bind_group_global();

			backend->bind_group_update_descriptor(pfd.bind_group_global, 0, pfd.buf_engine_global.get_hw_gpu());
			gfx_util::update_dummy_bind_group(pfd.bind_group_global, _gfx_data.dummy_texture, _gfx_data.dummy_texture_array, _gfx_data.dummy_texture_cube, _gfx_data.dummy_sampler, _gfx_data.dummy_ssbo, _gfx_data.dummy_ubo);

			pfd.bind_group_swapchain = backend->create_empty_bind_group();
			backend->bind_group_add_pointer(pfd.bind_group_swapchain, rpi_table_material, upi_material_texture1 + 1, false);

			backend->bind_group_update_pointer(pfd.bind_group_swapchain,
											   0,
											   {
												   {.resource = _world_renderer->get_output(i), .view = 0, .pointer_index = upi_material_texture0, .type = binding_type::texture_binding},

#ifdef USE_DEBUG_CONTROLLER
												   {.resource = _debug_controller.get_final_rt(i), .view = 0, .pointer_index = upi_material_texture1, .type = binding_type::texture_binding},
#endif

											   });

			_frame_allocator[i].init(1024 * 1024, 4);

			s_bind_group_global[i] = pfd.bind_group_global;
		}

		_buffer_queue.init();
		_texture_queue.init();
		_proxy_manager.init();
	}

	void renderer::uninit(render_event_stream& stream)
	{
		gfx_backend* backend = gfx_backend::get();

		// fetch any stale commands.
		_proxy_manager.fetch_render_events(stream);
		_proxy_manager.uninit();
		_world_renderer->uninit();
		delete _world_renderer;

		_shaders.swapchain.destroy();

#ifdef USE_DEBUG_CONTROLLER
		_debug_controller.uninit();
#endif

		_texture_queue.uninit();
		_buffer_queue.uninit();

		backend->destroy_bind_layout(_gfx_data.bind_layout_global);

		backend->destroy_resource(_gfx_data.dummy_ubo);
		backend->destroy_resource(_gfx_data.dummy_ssbo);
		backend->destroy_sampler(_gfx_data.dummy_sampler);
		backend->destroy_texture(_gfx_data.dummy_texture);
		backend->destroy_texture(_gfx_data.dummy_texture_array);
		backend->destroy_texture(_gfx_data.dummy_texture_cube);

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_bind_group(pfd.bind_group_swapchain);
			backend->destroy_semaphore(pfd.sem_frame.semaphore);
			backend->destroy_semaphore(pfd.sem_copy.semaphore);
			backend->destroy_command_buffer(pfd.cmd_gfx);
			backend->destroy_command_buffer(pfd.cmd_copy);
			backend->destroy_bind_group(pfd.bind_group_global);
			pfd.buf_engine_global.destroy();
			_frame_allocator[i].uninit();
		}

		backend->destroy_swapchain(_gfx_data.swapchain);
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
			_proxy_manager.flush_material_updates(i);
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

	void renderer::render(render_event_stream& stream, const vector2ui16& size)
	{
		gfx_backend* backend		= gfx_backend::get();
		const gfx_id queue_gfx		= backend->get_queue_gfx();
		const gfx_id queue_transfer = backend->get_queue_transfer();

		// Gate frame start to DXGI frame latency waitable for stable pacing
		backend->wait_for_swapchain_latency(_gfx_data.swapchain);

		/* access frame data */
		const uint8		frame_index	  = _gfx_data.frame_index;
		const gfx_id	layout_global = _gfx_data.bind_layout_global;
		per_frame_data& pfd			  = _pfd[frame_index];
		const gfx_id	render_target = _gfx_data.swapchain;

		bump_allocator& alloc = _frame_allocator[frame_index];
		alloc.reset();

		// Wait for frame's fence, then send any uploads needed.
		backend->wait_semaphore(pfd.sem_frame.semaphore, pfd.sem_frame.value);

		_proxy_manager.fetch_render_events(stream);
		_proxy_manager.flush_destroys(false);
		_proxy_manager.flush_material_updates(frame_index);

		/* access pfd */
		const gfx_id cmd_list		  = pfd.cmd_gfx;
		const gfx_id cmd_list_copy	  = pfd.cmd_copy;
		const gfx_id bg_global		  = pfd.bind_group_global;
		const gfx_id bg_swapchain	  = pfd.bind_group_swapchain;
		const gfx_id shader_swp		  = _shaders.swapchain.get_hw();
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
				.resource	= render_target,
				.flags		= barrier_flags::baf_is_swapchain,
				.from_state = resource_state::present,
				.to_state	= resource_state::render_target,
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

			backend->cmd_begin_render_pass_swapchain(cmd_list, {.color_attachments = attachment, .color_attachment_count = 1});
			backend->cmd_set_scissors(cmd_list, {.width = static_cast<uint16>(size.x), .height = static_cast<uint16>(size.y)});
			backend->cmd_set_viewport(cmd_list, {.width = static_cast<uint16>(size.x), .height = static_cast<uint16>(size.y)});
			backend->cmd_bind_group(cmd_list, {.group = bg_swapchain});
			backend->cmd_bind_pipeline(cmd_list, {.pipeline = shader_swp});
			backend->cmd_draw_instanced(cmd_list, {.vertex_count_per_instance = 6, .instance_count = 1});
			backend->cmd_end_render_pass(cmd_list, {});
		}

		// Rt -> Present Barrier
		{
			static_vector<barrier, 1> barriers;

			barriers.push_back({
				.resource	= render_target,
				.flags		= barrier_flags::baf_is_swapchain,
				.from_state = resource_state::render_target,
				.to_state	= resource_state::present,
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
		backend->queue_wait(queue_gfx, &sem_world, &sem_world_value, 1);

		backend->submit_commands(queue_gfx, &cmd_list, 1);

#ifndef SFG_PRODUCTION
		const int64 time_before = time::get_cpu_microseconds();
#endif

		backend->present(&render_target, 1);
		_gfx_data.frame_index = backend->get_back_buffer_index(_gfx_data.swapchain);

		// SFG_TRACE("frame index {0}", (uint32)_gfx_data.frame_index);
#ifndef SFG_PRODUCTION
		const int64 present_time = time::get_cpu_microseconds() - time_before;
		frame_info::s_present_time_micro.store(static_cast<double>(present_time));
#endif

		backend->queue_signal(queue_gfx, &sem_frame, &next_frame_value, 1);
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
		VERIFY_THREAD_MAIN();
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

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->bind_group_update_pointer(pfd.bind_group_swapchain,
											   0,
											   {
												   {.resource = _world_renderer->get_output(i), .view = 0, .pointer_index = upi_material_texture0, .type = binding_type::texture_binding},
#ifdef USE_DEBUG_CONTROLLER
												   {.resource = _debug_controller.get_final_rt(i), .view = 0, .pointer_index = upi_material_texture1, .type = binding_type::texture_binding},
#endif
											   });
		}
	}

	void renderer::on_swapchain_flags(uint8 flags)
	{
		VERIFY_THREAD_MAIN();

		gfx_backend* backend = gfx_backend::get();

		_swapchain_flags = flags;

		backend->recreate_swapchain({
			.size	   = _base_size,
			.swapchain = _gfx_data.swapchain,
			.flags	   = _swapchain_flags,
		});
	}

}
