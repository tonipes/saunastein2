// Copyright (c) 2025 Inan Evin

#include "render_pass_shadows.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/buffer_queue.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/renderer.hpp"
#include "gfx/engine_shaders.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "world/world.hpp"
#include "resources/vertex.hpp"
#include "math/vector2ui16.hpp"
#include "math/math.hpp"
#include "resources/shader_raw.hpp"

namespace SFG
{
	void render_pass_shadows::init(const init_params& params)
	{
		_alloc.init(params.alloc, params.alloc_size);

		gfx_backend* backend = gfx_backend::get();

		for (uint8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			for (uint8 j = 0; j < SHADOWS_MAX_CMD_BUFFERS; j++)
				pfd.cmd_buffers[j] = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "shadows_cmd"});

			const uint32 base = i * 4;
		}

		for (uint32 i = 0; i < SHADOW_PASSES_COUNT; i++)
		{
			pass& p = _passes[i];

			for (uint8 j = 0; j < BACK_BUFFER_COUNT; j++)
			{
				p.ubos[j].create_hw({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "shadows_ubo"});

				// p.bind_groups[j] = backend->create_empty_bind_group();
				// backend->bind_group_add_pointer(p.bind_groups[j], rpi_table_render_pass, upi_render_pass_texture4 + 1, false);
				// backend->bind_group_update_pointer(p.bind_groups[j],
				//								   0,
				//								   {
				//									   {.resource = p.ubos[j].get_hw_gpu(), .view = 0, .pointer_index = upi_render_pass_ubo0, .type = binding_type::ubo},
				//									   {.resource = params.entities[i], .view = 0, .pointer_index = upi_render_pass_ssbo0, .type = binding_type::ssbo},
				//									   {.resource = params.bones[i], .view = 0, .pointer_index = upi_render_pass_ssbo1, .type = binding_type::ssbo},
				//								   });
			} //
		}
	}

	void render_pass_shadows::uninit()
	{
		_alloc.uninit();

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			for (uint8 j = 0; j < SHADOWS_MAX_CMD_BUFFERS; j++)
				backend->destroy_command_buffer(pfd.cmd_buffers[j]);

			// backend->destroy_bind_group(pfd.bind_group);
		}
		for (uint32 i = 0; i < SHADOW_PASSES_COUNT; i++)
		{
			pass& p = _passes[i];

			for (uint8 j = 0; j < BACK_BUFFER_COUNT; j++)
			{
				p.ubos[j].destroy();
				// backend->destroy_bind_group(p.bind_groups[j]);
			}
		}
	}

	void render_pass_shadows::prepare(proxy_manager& pm, uint8 frame_index)
	{
		_passes.resize(0);
		const uint32 dirs_peak = pm.get_peak_dir_lights();
		auto&		 dirs	   = *pm.get_dir_lights();
	}

	void render_pass_shadows::render(const render_params& p)
	{
		gfx_backend*	backend	   = gfx_backend::get();
		per_frame_data& pfd		   = _pfd[p.frame_index];
		const gfx_id	cmd_buffer = pfd.cmd_buffers[p.cmd_index];

		const uint32 pass_count	 = static_cast<uint32>(_passes.size());
		const uint32 base_count	 = pass_count / (uint32)SHADOWS_MAX_CMD_BUFFERS;
		const uint32 extra_count = pass_count % (uint32)SHADOWS_MAX_CMD_BUFFERS;

		const uint32 my_count = base_count + (p.cmd_index < extra_count ? 1 : 0);
		const uint32 base	  = base_count * p.cmd_index + extra_count + p.cmd_index;

		backend->reset_command_buffer(cmd_buffer);
		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});

		static_vector<barrier, 1> barriers;

		for (uint32 i = 0; i < my_count; i++)
		{
			const pass&	 pass_data		= _passes[base + i];
			const gfx_id target_texture = pass_data.texture;
			// const gfx_id bind_group		= pass_data.bind_groups[p.frame_index];

			barriers.resize(0);
			barriers.push_back({
				.resource	 = target_texture,
				.flags		 = barrier_flags::baf_is_texture,
				.from_states = resource_state::resource_state_common,
				.to_states	 = resource_state::resource_state_depth_write,
			});

			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= barriers.data(),
									 .barrier_count = static_cast<uint16>(barriers.size()),
								 });

			BEGIN_DEBUG_EVENT(backend, cmd_buffer, "shadow_pass");

			backend->cmd_begin_render_pass_depth_only(cmd_buffer,
													  {
														  .depth_stencil_attachment =
															  {
																  .texture		  = target_texture,
																  .clear_stencil  = 0,
																  .clear_depth	  = 0.0f,
																  .depth_load_op  = load_op::clear,
																  .depth_store_op = store_op::store,
																  .view_index	  = 0,
															  },
													  });

			// backend->cmd_bind_group(cmd_buffer, {.group = bind_group});
			backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<uint16>(p.size.x), .height = static_cast<uint16>(p.size.y)});
			backend->cmd_set_viewport(cmd_buffer,
									  {
										  .min_depth = 0.0f,
										  .max_depth = 1.0f,
										  .width	 = static_cast<uint16>(p.size.x),
										  .height	 = static_cast<uint16>(p.size.y),

									  });

			gfx_id last_bound_group	   = NULL_GFX_ID;
			gfx_id last_bound_pipeline = NULL_GFX_ID;
			gfx_id last_vtx			   = NULL_GFX_ID;
			gfx_id last_idx			   = NULL_GFX_ID;

			auto bind = [&](gfx_id group, gfx_id pipeline, gfx_id vtx, gfx_id idx) {
				if (vtx != last_vtx)
				{
					last_vtx = vtx;
					backend->cmd_bind_vertex_buffers(cmd_buffer, {.buffer = vtx, .vertex_size = sizeof(vertex_static)});
				}

				if (idx != last_idx)
				{
					last_idx = idx;
					backend->cmd_bind_index_buffers(cmd_buffer, {.buffer = idx, .index_size = static_cast<uint8>(sizeof(primitive_index))});
				}
				if (pipeline != last_bound_pipeline)
				{
					last_bound_pipeline = pipeline;
					backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = pipeline});
				}

				if (group != last_bound_group)
				{
					last_bound_group = group;
					backend->cmd_bind_group(cmd_buffer, {.group = group});
				}
			};

			for (const indexed_draw& draw : pass_data.draws)
			{
				// bind(draw.bind_group, draw.pipeline, draw.vertex_buffer, draw.idx_buffer);

				// backend->cmd_bind_constants(cmd_buffer, {.data = (void*)&draw.constants, .offset = 0, .count = 4});
				backend->cmd_draw_indexed_instanced(cmd_buffer,
													{
														.index_count_per_instance = draw.index_count,
														.instance_count			  = draw.instance_count,
														.start_index_location	  = draw.start_index,
														.base_vertex_location	  = draw.base_vertex,
														.start_instance_location  = draw.start_instance,
													});
			}

			backend->cmd_end_render_pass(cmd_buffer, {});

			barriers.resize(0);
			barriers.push_back({
				.resource	 = target_texture,
				.flags		 = barrier_flags::baf_is_texture,
				.from_states = resource_state::resource_state_depth_write,
				.to_states	 = resource_state::resource_state_depth_read | resource_state::resource_state_ps_resource,
			});
			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= barriers.data(),
									 .barrier_count = static_cast<uint16>(barriers.size()),
								 });

			END_DEBUG_EVENT(backend, cmd_buffer);
		}

		backend->close_command_buffer(cmd_buffer);
	}
}
