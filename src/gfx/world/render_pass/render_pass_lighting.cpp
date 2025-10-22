// Copyright (c) 2025 Inan Evin

#include "render_pass_lighting.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/buffer_queue.hpp"
#include "gfx/world/world_render_data.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/renderer.hpp"
#include "gfx/engine_shaders.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "world/world.hpp"
#include "resources/vertex.hpp"
#include "math/vector2ui16.hpp"
#include "resources/shader_raw.hpp"

namespace SFG
{
	void render_pass_lighting::init(const init_params& params)
	{
		_alloc.init(params.alloc, params.alloc_size);

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer			= backend->create_command_buffer({.type = command_type::graphics, .debug_name = "lighting_cmd"});
			pfd.semaphore.semaphore = backend->create_semaphore();
			pfd.ubo.create_hw({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "lighting_ubo"});

			const uint32 base = i * 4;

			pfd.bind_group = backend->create_empty_bind_group();
			backend->bind_group_add_pointer(pfd.bind_group, rpi_table_render_pass, upi_render_pass_texture3 + 1, false);
			backend->bind_group_update_pointer(pfd.bind_group,
											   0,
											   {
												   {.resource = pfd.ubo.get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ubo0, .type = binding_type::ubo},
												   {.resource = params.gbuffer_textures[base + 0], .view = 0, .pointer_index = upi_render_pass_texture0, .type = binding_type::texture_binding},
												   {.resource = params.gbuffer_textures[base + 1], .view = 0, .pointer_index = upi_render_pass_texture1, .type = binding_type::texture_binding},
												   {.resource = params.gbuffer_textures[base + 2], .view = 0, .pointer_index = upi_render_pass_texture2, .type = binding_type::texture_binding},
												   {.resource = params.gbuffer_textures[base + 3], .view = 0, .pointer_index = upi_render_pass_texture3, .type = binding_type::texture_binding},
											   });
		}

		create_textures(params.size, params.gbuffer_textures);

		_shader_lighting = engine_shaders::get().get_shader(engine_shader_type::engine_shader_type_world_lighting).get_hw();

#ifdef SFG_TOOLMODE
		engine_shaders::get().add_reload_listener([this](engine_shader_type type, shader_direct& sh) {
			if (type == engine_shader_type::engine_shader_type_world_lighting)
			{
				_shader_lighting = sh.get_hw();
				return;
			}
		});
#endif
	}

	void render_pass_lighting::uninit()
	{
		_alloc.uninit();

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_command_buffer(pfd.cmd_buffer);
			backend->destroy_bind_group(pfd.bind_group);
			backend->destroy_semaphore(pfd.semaphore.semaphore);
			pfd.ubo.destroy();
		}

		destroy_textures();
	}

	void render_pass_lighting::prepare(proxy_manager& pm, view& camera_view, uint8 frame_index)
	{
		per_frame_data& pfd		 = _pfd[frame_index];
		const ubo		ubo_data = {
				  .ambient_color = vector4(0.5f, 0.0f, 0.0f, 1.0f),
		  };

		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_lighting::render(const render_params& p)
	{
		gfx_backend*	backend		  = gfx_backend::get();
		per_frame_data& pfd			  = _pfd[p.frame_index];
		const gfx_id	queue_gfx	  = backend->get_queue_gfx();
		const gfx_id	cmd_buffer	  = pfd.cmd_buffer;
		const gfx_id	color_texture = pfd.render_target;
		const gfx_id	rp_bind_group = pfd.bind_group;
		const gfx_id	sh			  = _shader_lighting;
		_alloc.reset();

		static_vector<barrier, 1> barriers;
		static_vector<barrier, 1> barriers_after;

		barriers.push_back({
			.resource	= color_texture,
			.flags		= barrier_flags::baf_is_texture,
			.from_state = resource_state::ps_resource,
			.to_state	= resource_state::render_target,
		});

		barriers_after.push_back({
			.resource	= color_texture,
			.flags		= barrier_flags::baf_is_texture,
			.from_state = resource_state::render_target,
			.to_state	= resource_state::ps_resource,
		});

		backend->reset_command_buffer(cmd_buffer);
		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<uint16>(barriers.size()),
							 });

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "lighting_pass");

		render_pass_color_attachment* attachments = _alloc.allocate<render_pass_color_attachment>(1);
		render_pass_color_attachment& att		  = attachments[0];
		att.clear_color							  = vector4(0, 0, 0, 1.0f);
		att.load_op								  = load_op::clear;
		att.store_op							  = store_op::store;
		att.texture								  = color_texture;

		backend->cmd_begin_render_pass(cmd_buffer,
									   {
										   .color_attachments	   = attachments,
										   .color_attachment_count = 1,
									   });

		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});
		backend->cmd_bind_group(cmd_buffer, {.group = rp_bind_group});
		backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<uint16>(p.size.x), .height = static_cast<uint16>(p.size.y)});
		backend->cmd_set_viewport(cmd_buffer,
								  {
									  .min_depth = 0.0f,
									  .max_depth = 1.0f,
									  .width	 = static_cast<uint16>(p.size.x),
									  .height	 = static_cast<uint16>(p.size.y),

								  });
		backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = sh});
		backend->cmd_draw_instanced(cmd_buffer,
									{
										.vertex_count_per_instance = 6,
										.instance_count			   = 1,
									});

		backend->cmd_end_render_pass(cmd_buffer, {});
		END_DEBUG_EVENT(backend, cmd_buffer);

		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers_after.data(),
								 .barrier_count = static_cast<uint16>(barriers_after.size()),
							 });

		backend->close_command_buffer(cmd_buffer);
	}

	void render_pass_lighting::resize(const vector2ui16& size, gfx_id* depth_textures)
	{
		destroy_textures();
		create_textures(size, depth_textures);
	}

	void render_pass_lighting::destroy_textures()
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_texture(pfd.render_target);
		}
	}

	void render_pass_lighting::create_textures(const vector2ui16& sz, gfx_id* gbuffer_textures)
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.render_target = backend->create_texture({
				.texture_format = render_target_definitions::get_format_lighting(),
				.size			= sz,
				.flags			= texture_flags::tf_render_target | texture_flags::tf_is_2d | texture_flags::tf_sampled,
				.clear_values	= {0.0f, 0.0f, 0.0f, 1.0f},
				.debug_name		= "lighting_rt",
			});

			const uint32 base = i * 4;
			backend->bind_group_update_pointer(pfd.bind_group,
											   0,
											   {
												   {.resource = pfd.ubo.get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ubo0, .type = binding_type::ubo},
												   {.resource = gbuffer_textures[base + 0], .view = 0, .pointer_index = upi_render_pass_texture0, .type = binding_type::texture_binding},
												   {.resource = gbuffer_textures[base + 1], .view = 0, .pointer_index = upi_render_pass_texture1, .type = binding_type::texture_binding},
												   {.resource = gbuffer_textures[base + 2], .view = 0, .pointer_index = upi_render_pass_texture2, .type = binding_type::texture_binding},
												   {.resource = gbuffer_textures[base + 3], .view = 0, .pointer_index = upi_render_pass_texture3, .type = binding_type::texture_binding},
											   });
		}
	}
}
