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

#include "render_pass_physics_debug.hpp"
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

#include "world/world.hpp"
#include "physics/physics_world.hpp"
#include "physics/physics_debug_renderer.hpp"
#include <Jolt/Physics/PhysicsSystem.h>
#include <tracy/Tracy.hpp>

namespace SFG
{
	void render_pass_physics_debug::init(const vector2ui16& size)
	{
		_renderer = new physics_debug_renderer();

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "phy_debug_cmd"});
			pfd.ubo.create_hw({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "phy_debug_ubo"});

			pfd.triangle_vertices.create(
				{
					.size		= physics_debug_renderer::MAX_TRI_VERTICES_SIZE,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "phy_dbg_tri_vtx",
				},
				{
					.size		= physics_debug_renderer::MAX_TRI_VERTICES_SIZE,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_vertex_buffer,
					.debug_name = "phy_dbg_tri_vtx",
				});

			pfd.line_vertices.create(
				{
					.size		= physics_debug_renderer::MAX_LINE_VERTICES_SIZE,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "phy_dbg_tri_vtx",
				},
				{
					.size		= physics_debug_renderer::MAX_LINE_VERTICES_SIZE,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_vertex_buffer,
					.debug_name = "phy_dbg_line_vtx",
				});

			pfd.triangle_indices.create(
				{
					.size		= physics_debug_renderer::MAX_TRI_INDICES_SIZE,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "phy_dbg_tri_index",
				},
				{
					.size		= physics_debug_renderer::MAX_TRI_INDICES_SIZE,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_index_buffer,
					.debug_name = "phy_dbg_tri_index",
				});

			pfd.line_indices.create(
				{
					.size		= physics_debug_renderer::MAX_LINE_INDICES_SIZE,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "phy_dbg_line_index",
				},
				{
					.size		= physics_debug_renderer::MAX_LINE_INDICES_SIZE,
					.flags		= resource_flags::rf_gpu_only | resource_flags::rf_index_buffer,
					.debug_name = "phy_dbg_line_index",
				});
		}

		_shader_debug_default = engine_shaders::get().get_shader(engine_shader_type::engine_shader_debug_default).get_hw();
		_shader_debug_line	  = engine_shaders::get().get_shader(engine_shader_type::engine_shader_debug_line).get_hw();

#ifdef SFG_TOOLMODE
		engine_shaders::get().add_reload_listener([this](engine_shader_type type, shader_direct& sh) {
			if (type == engine_shader_type::engine_shader_debug_default)
			{
				_shader_debug_default = sh.get_hw();
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

	void render_pass_physics_debug::uninit()
	{
		ZoneScoped;

		delete _renderer;
		_renderer = nullptr;

		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_command_buffer(pfd.cmd_buffer);
			pfd.ubo.destroy();
			pfd.triangle_vertices.destroy();
			pfd.line_vertices.destroy();
			pfd.triangle_indices.destroy();
			pfd.line_indices.destroy();
		}
	}

	void render_pass_physics_debug::tick(world& w)
	{
		_renderer->reset();

		JPH::BodyManager::DrawSettings ds = {};
		ds.mDrawShape					  = true;
		ds.mDrawVelocity				  = true;

		w.get_physics_world().get_system()->DrawBodies(ds, _renderer);

		_renderer->end();
	}

	void render_pass_physics_debug::prepare(const view& main_camera_view, const vector2ui16& resolution, uint8 frame_index)
	{
		ZoneScoped;

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

		const double_buffered_swap& triangle_vertices = _renderer->get_triangle_vertices();
		const double_buffered_swap& triangle_indices  = _renderer->get_triangle_indices();
		const double_buffered_swap& line_vertices	  = _renderer->get_line_vertices();
		const double_buffered_swap& line_indices	  = _renderer->get_line_indices();

		const uint32 vtx_count_triangle = _renderer->get_vertex_count_triangle();
		const uint32 vtx_count_line		= _renderer->get_vertex_count_line();
		const uint32 idx_count_triangle = _renderer->get_index_count_triangle();
		const uint32 idx_count_line		= _renderer->get_index_count_line();

		pfd._triangle_idx_count = idx_count_triangle;
		pfd._line_idx_count		= idx_count_line;

		static_vector<barrier, 4> barriers;
		static_vector<barrier, 4> barriers_after;

		if (vtx_count_triangle != 0)
		{
			triangle_vertices.read((void*)pfd.triangle_vertices.get_mapped(), vtx_count_triangle * sizeof(vertex_simple));
			triangle_indices.read((void*)pfd.triangle_indices.get_mapped(), idx_count_triangle * sizeof(uint32));

			barriers.push_back({
				.resource	 = pfd.triangle_vertices.get_hw_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_vertex_cbv,
				.to_states	 = resource_state::resource_state_copy_dest,
			});

			barriers.push_back({
				.resource	 = pfd.triangle_indices.get_hw_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_index_buffer,
				.to_states	 = resource_state::resource_state_copy_dest,
			});

			barriers_after.push_back({
				.resource	 = pfd.triangle_vertices.get_hw_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_vertex_cbv,
			});

			barriers_after.push_back({
				.resource	 = pfd.triangle_indices.get_hw_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_index_buffer,
			});
		}

		if (vtx_count_line != 0)
		{
			line_vertices.read((void*)pfd.line_vertices.get_mapped(), vtx_count_line * sizeof(vertex_3d_line));
			line_indices.read((void*)pfd.line_indices.get_mapped(), idx_count_line * sizeof(uint32));

			barriers.push_back({
				.resource	 = pfd.line_vertices.get_hw_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_vertex_cbv,
				.to_states	 = resource_state::resource_state_copy_dest,
			});

			barriers.push_back({
				.resource	 = pfd.line_indices.get_hw_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_index_buffer,
				.to_states	 = resource_state::resource_state_copy_dest,
			});

			barriers_after.push_back({
				.resource	 = pfd.line_vertices.get_hw_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_vertex_cbv,
			});

			barriers_after.push_back({
				.resource	 = pfd.line_indices.get_hw_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_index_buffer,
			});
		}

		if (vtx_count_triangle != 0 || vtx_count_line != 0)
		{
			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= barriers.data(),
									 .barrier_count = static_cast<uint16>(barriers.size()),
								 });

			if (vtx_count_line != 0)
			{

				pfd.line_vertices.copy_region(cmd_buffer, 0, sizeof(vertex_3d_line) * vtx_count_line);
				pfd.line_indices.copy_region(cmd_buffer, 0, sizeof(uint32) * idx_count_line);
			}

			if (vtx_count_triangle)
			{
				pfd.triangle_vertices.copy_region(cmd_buffer, 0, sizeof(vertex_simple) * vtx_count_triangle);
				pfd.triangle_indices.copy_region(cmd_buffer, 0, sizeof(uint32) * idx_count_triangle);
			}

			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= barriers_after.data(),
									 .barrier_count = static_cast<uint16>(barriers_after.size()),
								 });
		}
	}

	void render_pass_physics_debug::render(const render_params& p)
	{
		gfx_backend*	backend			 = gfx_backend::get();
		per_frame_data& pfd				 = _pfd[p.frame_index];
		const gfx_id	texture			 = p.input_texture;
		const gfx_id	depth			 = p.depth_texture;
		const gfx_id	cmd_buffer		 = pfd.cmd_buffer;
		const gfx_id	vtx_buffer_tri	 = pfd.triangle_vertices.get_hw_gpu();
		const gfx_id	idx_buffer_tri	 = pfd.triangle_indices.get_hw_gpu();
		const gfx_id	vtx_buffer_line	 = pfd.line_vertices.get_hw_gpu();
		const gfx_id	idx_buffer_line	 = pfd.line_indices.get_hw_gpu();
		const gpu_index gpu_index_rp_ubo = pfd.ubo.get_gpu_index();
		const uint32	tri_idx_count	 = pfd._triangle_idx_count;
		const uint32	line_idx_count	 = pfd._line_idx_count;

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

		if (tri_idx_count != 0)
		{
			backend->cmd_bind_vertex_buffers(cmd_buffer, {.buffer = vtx_buffer_tri, .vertex_size = sizeof(vertex_simple)});
			backend->cmd_bind_index_buffers(cmd_buffer, {.buffer = idx_buffer_tri, .index_size = sizeof(uint32)});
			backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = _shader_debug_default});
			backend->cmd_draw_indexed_instanced(cmd_buffer,
												{
													.index_count_per_instance = tri_idx_count,
													.instance_count			  = 1,
													.start_index_location	  = 0,
													.base_vertex_location	  = 0,
													.start_instance_location  = 0,
												});
		}

		if (line_idx_count != 0)
		{
			backend->cmd_bind_vertex_buffers(cmd_buffer, {.buffer = vtx_buffer_line, .vertex_size = sizeof(vertex_3d_line)});
			backend->cmd_bind_index_buffers(cmd_buffer, {.buffer = idx_buffer_line, .index_size = sizeof(uint32)});
			backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = _shader_debug_line});
			backend->cmd_draw_indexed_instanced(cmd_buffer,
												{
													.index_count_per_instance = line_idx_count,
													.instance_count			  = 1,
													.start_index_location	  = 0,
													.base_vertex_location	  = 0,
													.start_instance_location  = 0,
												});
		}

		backend->cmd_end_render_pass(cmd_buffer, {});
		END_DEBUG_EVENT(backend, cmd_buffer);

		static_vector<barrier, 2> barriers;

		barriers.push_back({
			.resource	 = p.depth_texture,
			.flags		 = barrier_flags::baf_is_texture,
			.from_states = resource_state::resource_state_depth_read | resource_state::resource_state_ps_resource | resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_common,
		});

		barriers.push_back({
			.resource	 = p.input_texture,
			.flags		 = barrier_flags::baf_is_texture,
			.from_states = resource_state::resource_state_render_target,
			.to_states	 = resource_state::resource_state_ps_resource | resource_state::resource_state_non_ps_resource,
		});

		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<uint16>(barriers.size()),
							 });

		backend->close_command_buffer(cmd_buffer);
	}

}
