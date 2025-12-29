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

#include "render_pass_particles.hpp"
#include "math/math.hpp"

// gfx
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/engine_shaders.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "gfx/world/view.hpp"
#include "gfx/texture_queue.hpp"

#include <tracy/Tracy.hpp>

namespace SFG
{

	void render_pass_particles::init()
	{
		gfx_backend* backend = gfx_backend::get();

		// ofd
		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer		   = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "particles_gfx"});
			pfd.cmd_buffer_compute = backend->create_command_buffer({.type = command_type::compute, .debug_name = "particles_cmp"});
			pfd.ubo.create({.size = sizeof(pass_params), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "particles_ubo"});
		}

		_shader_clear		= engine_shaders::get().get_shader(engine_shader_type::engine_shader_particle_clear).get_hw();
		_shader_simulate	= engine_shaders::get().get_shader(engine_shader_type::engine_shader_particle_sim).get_hw();
		_shader_emit		= engine_shaders::get().get_shader(engine_shader_type::engine_shader_particle_emit).get_hw();
		_shader_write_count = engine_shaders::get().get_shader(engine_shader_type::engine_shader_particle_write_count).get_hw();
		_shader_count		= engine_shaders::get().get_shader(engine_shader_type::engine_shader_particle_count).get_hw();
		_shader_swap		= engine_shaders::get().get_shader(engine_shader_type::engine_shader_particle_swap).get_hw();

#ifdef SFG_TOOLMODE
		engine_shaders::get().add_reload_listener([this](engine_shader_type type, shader_direct& sh) {
			if (type == engine_shader_type::engine_shader_particle_clear)
			{
				_shader_clear = sh.get_hw();
				return;
			}
			if (type == engine_shader_type::engine_shader_particle_sim)
			{
				_shader_simulate = sh.get_hw();
				return;
			}

			if (type == engine_shader_type::engine_shader_particle_emit)
			{
				_shader_emit = sh.get_hw();
				return;
			}

			if (type == engine_shader_type::engine_shader_particle_write_count)
			{
				_shader_write_count = sh.get_hw();
				return;
			}

			if (type == engine_shader_type::engine_shader_particle_count)
			{
				_shader_count = sh.get_hw();
				return;
			}

			if (type == engine_shader_type::engine_shader_particle_swap)
			{
				_shader_swap = sh.get_hw();
				return;
			}
		});
#endif
	}

	void render_pass_particles::uninit()
	{
		gfx_backend* backend = gfx_backend::get();

		for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_command_buffer(pfd.cmd_buffer);
			backend->destroy_command_buffer(pfd.cmd_buffer_compute);
			pfd.ubo.destroy();
		}
	}

	void render_pass_particles::prepare(uint8 frame_index, proxy_manager& pm, const view& main_camera_view)
	{
		ZoneScoped;

		per_frame_data& pfd = _pfd[frame_index];
		// const ubo		ubo_data = {
		// 		  .filter_radius = 0.01f,
		//   };
		// pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_particles::compute(uint8 frame_index)
	{
		ZoneScoped;
		gfx_backend*	backend	   = gfx_backend::get();
		per_frame_data& pfd		   = _pfd[frame_index];
		const gfx_id	cmd_buffer = pfd.cmd_buffer_compute;

		backend->reset_command_buffer(cmd_buffer);

		backend->close_command_buffer(cmd_buffer);
	}

	void render_pass_particles::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	backend	   = gfx_backend::get();
		per_frame_data& pfd		   = _pfd[p.frame_index];
		const gfx_id	cmd_buffer = pfd.cmd_buffer;

		const gpu_index gpu_index_ubo = pfd.ubo.get_index();

		backend->reset_command_buffer(cmd_buffer);

		// static_vector<barrier, 2> barriers_uav;
		// barriers_uav.push_back({
		// 	.resource = output,
		// 	.flags	  = barrier_flags::baf_is_texture,
		// });
		//
		// static_vector<barrier, 2> barriers;
		// barriers.push_back({
		// 	.resource	 = output,
		// 	.flags		 = barrier_flags::baf_is_texture,
		// 	.from_states = resource_state::resource_state_common,
		// 	.to_states	 = resource_state::resource_state_non_ps_resource,
		// });
		//
		// backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});
		// barriers.resize(0);
		// backend->cmd_bind_layout_compute(cmd_buffer, {.layout = p.global_layout_compute});
		// backend->cmd_bind_group_compute(cmd_buffer, {.group = p.global_group});
		// backend->cmd_bind_constants_compute(cmd_buffer, {.data = (uint8*)&gpu_index_ubo, .offset = constant_index_rp_constant0, .count = 1, .param_index = rpi_constants});
		//
		// backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = shader_bloom_downsample});
		//
		// gpu_index downsample_input	= p.gpu_index_lighting;
		// gpu_index downsample_output = pfd.gpu_index_downsample_uav[0];
		//
		// for (uint32 i = 0; i < MIPS_DS; i++)
		// {
		// 	BEGIN_DEBUG_EVENT(backend, cmd_buffer, "bloom_downsample");
		// 	const uint32 group_size_x = 8;
		// 	const uint32 group_size_y = 8;
		// 	const uint32 half_w		  = res.x * math::pow(0.5f, static_cast<float>(i + 1));
		// 	const uint32 half_h		  = res.y * math::pow(0.5f, static_cast<float>(i + 1));
		// 	const uint32 gsx		  = (group_size_x + half_w - 1) / group_size_x;
		// 	const uint32 gsy		  = (group_size_y + half_h - 1) / group_size_y;
		//
		// 	{
		// 		const uint32 constants[5] = {half_w, half_h, downsample_input, downsample_output, i};
		// 		backend->cmd_bind_constants_compute(cmd_buffer, {.data = (uint8*)&constants, .offset = constant_index_rp_constant1, .count = 5, .param_index = rpi_constants});
		// 	}
		//
		// 	backend->cmd_dispatch(cmd_buffer, {.group_size_x = gsx, .group_size_y = gsy, .group_size_z = 1});
		// 	END_DEBUG_EVENT(backend, cmd_buffer);
		//
		// 	if (i < MIPS_DS - 1)
		// 	{
		// 		downsample_input  = pfd.gpu_index_downsample_srv[i];
		// 		downsample_output = pfd.gpu_index_downsample_uav[i + 1];
		// 	}
		//
		// 	backend->cmd_barrier_uav(cmd_buffer, {.barriers = barriers_uav.data(), .barrier_count = static_cast<uint16>(barriers_uav.size())});
		// }
		//
		// backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = shader_bloom_upsample});
		//
		// gpu_index upsample_input  = pfd.gpu_index_downsample_srv[MIPS_DS - 1];
		// gpu_index upsample_output = pfd.gpu_index_upsample_uav[MIPS_DS - 1];
		//
		// for (int32 i = MIPS_DS - 1; i >= 0; --i)
		// {
		// 	BEGIN_DEBUG_EVENT(backend, cmd_buffer, "bloom_upsample");
		// 	const uint32 group_size_x = 8;
		// 	const uint32 group_size_y = 8;
		// 	const uint32 half_w		  = res.x * math::pow(0.5f, static_cast<float>(i));
		// 	const uint32 half_h		  = res.y * math::pow(0.5f, static_cast<float>(i));
		// 	const uint32 gsx		  = (group_size_x + half_w - 1) / group_size_x;
		// 	const uint32 gsy		  = (group_size_y + half_h - 1) / group_size_y;
		//
		// 	{
		// 		const uint32 constants[4] = {half_w, half_h, upsample_input, upsample_output};
		// 		backend->cmd_bind_constants_compute(cmd_buffer, {.data = (uint8*)&constants, .offset = constant_index_rp_constant1, .count = 4, .param_index = rpi_constants});
		// 	}
		//
		// 	backend->cmd_dispatch(cmd_buffer, {.group_size_x = gsx, .group_size_y = gsy, .group_size_z = 1});
		// 	END_DEBUG_EVENT(backend, cmd_buffer);
		//
		// 	if (i != 0)
		// 	{
		// 		upsample_input	= pfd.gpu_index_upsample_srv[i];
		// 		upsample_output = pfd.gpu_index_upsample_uav[i - 1];
		// 	}
		//
		// 	backend->cmd_barrier_uav(cmd_buffer, {.barriers = barriers_uav.data(), .barrier_count = static_cast<uint16>(barriers_uav.size())});
		// }
		//
		// barriers.push_back({
		// 	.resource	 = output,
		// 	.flags		 = barrier_flags::baf_is_texture,
		// 	.from_states = resource_state::resource_state_non_ps_resource,
		// 	.to_states	 = resource_state::resource_state_common,
		// });
		// backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});
		// barriers.resize(0);

		backend->close_command_buffer(cmd_buffer);
	}

}
