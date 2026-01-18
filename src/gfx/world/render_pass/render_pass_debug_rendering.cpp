/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "render_pass_debug_rendering.hpp"
#include "resources/vertex.hpp"
#include "math/math.hpp"

// gfx
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "gfx/world/view.hpp"
#include "gfx/world/renderable.hpp"
#include "gfx/world/renderable_collector.hpp"
#include "gfx/engine_shaders.hpp"

#include "resources/vertex.hpp"

#include "world/world.hpp"
#include "physics/physics_world.hpp"
#include "physics/physics_debug_renderer.hpp"
#include <tracy/Tracy.hpp>

namespace SFG
{
	void render_pass_debug::init(const vector2ui16& size, world& w)
	{
		_renderer = new physics_debug_renderer();
		_world	  = &w;
		_alloc.init(PASS_ALLOC_SIZE_DEBUG_RENDERING, 8);

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "phy_debug_cmd"});
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "phy_debug_ubo"});
			pfd.vtx_buffer_line.create(
				{
					.size		= world_debug_rendering::MAX_VERTEX_COUNT_LINE * sizeof(vertex_3d_line),
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "debug_vtx",
				},
				{
					.size		= world_debug_rendering::MAX_VERTEX_COUNT_LINE * sizeof(vertex_3d_line),
					.flags		= resource_flags::rf_vertex_buffer | resource_flags::rf_gpu_only,
					.debug_name = "debug_vtx",
				});

			pfd.vtx_buffer_gui.create(
				{
					.size		= world_debug_rendering::MAX_VERTEX_COUNT_GUI * sizeof(vertex_gui),
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "debug_vtx",
				},
				{
					.size		= world_debug_rendering::MAX_VERTEX_COUNT_GUI * sizeof(vertex_gui),
					.flags		= resource_flags::rf_vertex_buffer | resource_flags::rf_gpu_only,
					.debug_name = "debug_vtx",
				});

			pfd.vtx_buffer_tri.create(
				{
					.size		= world_debug_rendering::MAX_VERTEX_COUNT_TRI * sizeof(vertex_simple),
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "debug_vtx",
				},
				{
					.size		= world_debug_rendering::MAX_VERTEX_COUNT_TRI * sizeof(vertex_simple),
					.flags		= resource_flags::rf_vertex_buffer | resource_flags::rf_gpu_only,
					.debug_name = "debug_vtx",
				});

			pfd.idx_buffer.create(
				{
					.size		= world_debug_rendering::MAX_INDEX_COUNT * sizeof(primitive_index),
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "debug_idx",
				},
				{
					.size		= world_debug_rendering::MAX_INDEX_COUNT * sizeof(primitive_index),
					.flags		= resource_flags::rf_index_buffer | resource_flags::rf_gpu_only,
					.debug_name = "debug_idx",
				});
		}

		_shader_debug_triangle	  = engine_shaders::get().get_shader(engine_shader_type::engine_shader_debug_default).get_hw();
		_shader_debug_line		  = engine_shaders::get().get_shader(engine_shader_type::engine_shader_debug_line).get_hw();
		_shader_debug_gui_default = 0;
		_shader_debug_gui_text	  = 0;
		_shader_debug_gui_sdf	  = 0;

#ifdef SFG_TOOLMODE
		engine_shaders::get().add_reload_listener([this](engine_shader_type type, shader_direct& sh) {
			if (type == engine_shader_type::engine_shader_debug_default)
			{
				_shader_debug_triangle = sh.get_hw();
				return;
			}

			if (type == engine_shader_type::engine_shader_debug_line)
			{
				_shader_debug_line = sh.get_hw();
				return;
			}
		});
#endif
	}

	void render_pass_debug::uninit()
	{
		ZoneScoped;

		_alloc.uninit();

		delete _renderer;
		_renderer = nullptr;

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_command_buffer(pfd.cmd_buffer);
			pfd.ubo.destroy();
			pfd.vtx_buffer_line.destroy();
			pfd.vtx_buffer_tri.destroy();
			pfd.vtx_buffer_gui.destroy();
			pfd.idx_buffer.destroy();
		}
	}

	void render_pass_debug::tick()
	{
		_renderer->draw(*_world);
	}

	void render_pass_debug::prepare(const view& main_camera_view, const vector2ui16& resolution, uint8 frame_index)
	{
		ZoneScoped;

		_alloc.reset();
		_draw_stream.prepare(_alloc, MAX_DRAW_CALLS_DEBUG_RENDERING);

		gfx_backend*	backend	   = gfx_backend::get();
		per_frame_data& pfd		   = _pfd[frame_index];
		const gfx_id	cmd_buffer = pfd.cmd_buffer;

		backend->reset_command_buffer(cmd_buffer);

		const ubo ubo_data = {
			.view				   = main_camera_view.view_matrix,
			.proj				   = main_camera_view.proj_matrix,
			.view_proj			   = main_camera_view.view_proj_matrix,
			.resolution_and_planes = vector4(resolution.x, resolution.y, main_camera_view.near_plane, main_camera_view.far_plane),
		};
		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));

		world_debug_rendering&				   dbg_rnd = _world->get_debug_rendering();
		const world_debug_rendering::snapshot* snap	   = dbg_rnd.get_read_snapshot();

		if (snap == nullptr)
			return;

		const uint32 vtx_count_gui	= snap->vtx_count_gui;
		const uint32 vtx_count_line = snap->vtx_count_line;
		const uint32 vtx_count_tri	= snap->vtx_count_tri;
		const uint32 idx_count		= snap->idx_count;

		static_vector<barrier, 4> barriers;

		// -----------------------------------------------------------------------------
		// before copy
		// -----------------------------------------------------------------------------

		if (vtx_count_gui != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_vertex_cbv,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = pfd.vtx_buffer_gui.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			pfd.vtx_buffer_gui.buffer_data(0, snap->vertices_gui, sizeof(vertex_gui) * vtx_count_gui);
		}

		if (vtx_count_line != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_vertex_cbv,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = pfd.vtx_buffer_line.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			pfd.vtx_buffer_line.buffer_data(0, snap->vertices_line, sizeof(vertex_3d_line) * vtx_count_line);
		}

		if (vtx_count_tri != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_vertex_cbv,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = pfd.vtx_buffer_tri.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			pfd.vtx_buffer_tri.buffer_data(0, snap->vertices_tri, sizeof(vertex_simple) * vtx_count_tri);
		}

		if (idx_count != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_index_buffer,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = pfd.idx_buffer.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			pfd.idx_buffer.buffer_data(0, snap->indices, sizeof(primitive_index) * idx_count);
		}

		// -----------------------------------------------------------------------------
		// perform barräer
		// -----------------------------------------------------------------------------

		if (!barriers.empty())
		{
			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= barriers.data(),
									 .barrier_count = static_cast<uint16>(barriers.size()),
								 });
		}

		// -----------------------------------------------------------------------------
		// copy and post barriers
		// -----------------------------------------------------------------------------

		barriers.resize(0);
		if (vtx_count_gui != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_vertex_cbv,
				.resource	 = pfd.vtx_buffer_gui.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			pfd.vtx_buffer_gui.copy_region(cmd_buffer, 0, sizeof(vertex_gui) * vtx_count_gui);
		}

		if (vtx_count_line != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_vertex_cbv,
				.resource	 = pfd.vtx_buffer_line.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			pfd.vtx_buffer_line.copy_region(cmd_buffer, 0, sizeof(vertex_3d_line) * vtx_count_line);
		}

		if (vtx_count_tri != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_vertex_cbv,
				.resource	 = pfd.vtx_buffer_tri.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			pfd.vtx_buffer_tri.copy_region(cmd_buffer, 0, sizeof(vertex_simple) * vtx_count_tri);
		}

		if (idx_count != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_index_buffer,
				.resource	 = pfd.idx_buffer.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			pfd.idx_buffer.copy_region(cmd_buffer, 0, sizeof(primitive_index) * idx_count);
		}

		// -----------------------------------------------------------------------------
		// perform barräer
		// -----------------------------------------------------------------------------

		if (!barriers.empty())
		{
			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= barriers.data(),
									 .barrier_count = static_cast<uint16>(barriers.size()),
								 });
		}

		// -----------------------------------------------------------------------------
		// draw calls
		// -----------------------------------------------------------------------------

		const uint32 dc_count = snap->draw_call_count;
		for (uint32 i = 0; i < dc_count; i++)
		{
			debug_draw_call& dc = snap->draw_calls[i];

			gfx_id vertex_buffer_hw = pfd.vtx_buffer_line.get_gpu();
			gfx_id pipeline			= _shader_debug_line;
			uint16 vtx_size			= sizeof(vertex_3d_line);

			if (dc.vtx_buffer_index == 1)
			{
				vertex_buffer_hw = pfd.vtx_buffer_tri.get_gpu();
				pipeline		 = _shader_debug_triangle;
				vtx_size		 = sizeof(vertex_simple);
			}
			else if (dc.vtx_buffer_index == 2)
			{
				vertex_buffer_hw = pfd.vtx_buffer_gui.get_gpu();
				pipeline		 = _shader_debug_gui_default;
				vtx_size		 = sizeof(vertex_gui);
			}
			else if (dc.vtx_buffer_index == 3)
			{
				vertex_buffer_hw = pfd.vtx_buffer_gui.get_gpu();
				pipeline		 = _shader_debug_gui_text;
				vtx_size		 = sizeof(vertex_gui);
			}
			else if (dc.vtx_buffer_index == 4)
			{
				vertex_buffer_hw = pfd.vtx_buffer_gui.get_gpu();
				pipeline		 = _shader_debug_gui_sdf;
				vtx_size		 = sizeof(vertex_gui);
			}

			const gfx_id index_buffer_hw = pfd.idx_buffer.get_gpu();

			_draw_stream.add_command({
				.start_index			 = dc.start_index,
				.index_count			 = dc.index_count,
				.base_vertex			 = dc.base_vertex,
				.material_constant_index = NULL_GPU_INDEX,
				.texture_constant_index	 = NULL_GPU_INDEX,
				.font_index				 = NULL_GPU_INDEX,
				.vb_hw					 = vertex_buffer_hw,
				.ib_hw					 = index_buffer_hw,
				.pipeline_hw			 = pipeline,
				.vertex_size			 = vtx_size,
			});
		}

		_draw_stream.build();
	}

	void render_pass_debug::render(const render_params& p)
	{
		gfx_backend*	backend			 = gfx_backend::get();
		per_frame_data& pfd				 = _pfd[p.frame_index];
		const gfx_id	texture			 = p.input_texture;
		const gfx_id	depth			 = p.depth_texture;
		const gfx_id	cmd_buffer		 = pfd.cmd_buffer;
		const gpu_index gpu_index_rp_ubo = pfd.ubo.get_index();

		const render_pass_color_attachment att = {
			.clear_color = vector4(0, 0, 0, 1.0f),
			.texture	 = texture,
			.load_op	 = load_op::load,
			.store_op	 = store_op::store,
		};

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "physics_debug_pass");

		backend->cmd_begin_render_pass_depth_read_only(cmd_buffer,
													   {
														   .color_attachments = &att,
														   .depth_stencil_attachment =
															   {
																   .texture		   = depth,
																   .depth_load_op  = load_op::load,
																   .depth_store_op = store_op::store,
																   .view_index	   = 1,
															   },
														   .color_attachment_count = 1,
													   });

		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});

		const uint32 rp_constants[1] = {gpu_index_rp_ubo};
		backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)rp_constants, .offset = constant_index_rp_constant0, .count = 1, .param_index = rpi_constants});

		backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<uint16>(p.size.x), .height = static_cast<uint16>(p.size.y)});
		backend->cmd_set_viewport(cmd_buffer,
								  {
									  .min_depth = 0.0f,
									  .max_depth = 1.0f,
									  .width	 = static_cast<uint16>(p.size.x),
									  .height	 = static_cast<uint16>(p.size.y),

								  });

		_draw_stream.draw_no_clip(cmd_buffer);

		backend->cmd_end_render_pass(cmd_buffer, {});
		END_DEBUG_EVENT(backend, cmd_buffer);

		static_vector<barrier, 2> barriers;

		barriers.push_back({
			.from_states = resource_state::resource_state_depth_read | resource_state::resource_state_ps_resource | resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_common,
			.resource	 = p.depth_texture,
			.flags		 = barrier_flags::baf_is_texture,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_render_target,
			.to_states	 = resource_state::resource_state_ps_resource | resource_state::resource_state_non_ps_resource,
			.resource	 = p.input_texture,
			.flags		 = barrier_flags::baf_is_texture,
		});

		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<uint16>(barriers.size()),
							 });

		backend->close_command_buffer(cmd_buffer);
	}

}
