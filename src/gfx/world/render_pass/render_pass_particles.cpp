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
#include "math/random.hpp"

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

#include "common/system_info.hpp"

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
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "particles_ubo"});
			pfd.emit_counts.create(
				{
					.size		= sizeof(uint32) * MAX_WORLD_PARTICLES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "emit_counts",
				},
				{
					.size			 = sizeof(uint32) * MAX_WORLD_PARTICLES,
					.structure_size	 = sizeof(uint32),
					.structure_count = MAX_WORLD_PARTICLES,
					.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
					.debug_name		 = "emit_counts",
				});

			pfd.emit_arguments.create(
				{
					.size		= sizeof(particle_emit_args) * MAX_WORLD_COMP_PARTICLE_EMITTERS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "emit_counts",
				},
				{
					.size			 = sizeof(particle_emit_args) * MAX_WORLD_COMP_PARTICLE_EMITTERS,
					.structure_size	 = sizeof(particle_emit_args),
					.structure_count = MAX_WORLD_COMP_PARTICLE_EMITTERS,
					.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
					.debug_name		 = "emit_counts",
				});

			pfd.system_data.create(
				{
					.size		= sizeof(particle_system_data) * MAX_WORLD_COMP_PARTICLE_EMITTERS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "particle_system_data",
				},
				{
					.size			 = sizeof(particle_system_data) * MAX_WORLD_COMP_PARTICLE_EMITTERS,
					.structure_size	 = sizeof(particle_system_data),
					.structure_count = MAX_WORLD_COMP_PARTICLE_EMITTERS,
					.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
					.debug_name		 = "particle_system_data",
				});

			pfd.states.create(
				{
					.size		= sizeof(particle_state) * MAX_WORLD_PARTICLES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "particle_states",
				},
				{
					.size			 = sizeof(particle_state) * MAX_WORLD_PARTICLES,
					.structure_size	 = sizeof(particle_state),
					.structure_count = MAX_WORLD_PARTICLES,
					.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
					.debug_name		 = "particle_states",
				});

			pfd.indirect_arguments.create(
				{
					.size		= sizeof(particle_indirect_args) * MAX_WORLD_COMP_PARTICLE_EMITTERS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "particle_render_args",
				},
				{
					.size			 = sizeof(particle_indirect_args) * MAX_WORLD_COMP_PARTICLE_EMITTERS,
					.structure_size	 = sizeof(particle_indirect_args),
					.structure_count = MAX_WORLD_COMP_PARTICLE_EMITTERS,
					.flags			 = resource_flags::rf_indirect_buffer | resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
					.debug_name		 = "particle_render_args",
				});

			pfd.sim_count_indirect_arguments.create(
				{
					.size		= sizeof(particle_sim_count_args),
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "particle_sim_count_args",
				},
				{
					.size			 = sizeof(particle_sim_count_args),
					.structure_size	 = sizeof(particle_sim_count_args),
					.structure_count = 1,
					.flags			 = resource_flags::rf_indirect_buffer | resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
					.debug_name		 = "particle_sim_count_args",
				});

			pfd.alive_list_a.create(
				{
					.size		= sizeof(uint32) * MAX_WORLD_PARTICLES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "particle_alive_a",
				},
				{
					.size			 = sizeof(uint32) * MAX_WORLD_PARTICLES,
					.structure_size	 = sizeof(uint32),
					.structure_count = MAX_WORLD_PARTICLES,
					.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
					.debug_name		 = "particle_alive_a",
				});

			pfd.alive_list_b.create(
				{
					.size		= sizeof(uint32) * MAX_WORLD_PARTICLES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "particle_alive_b",
				},
				{
					.size			 = sizeof(uint32) * MAX_WORLD_PARTICLES,
					.structure_size	 = sizeof(uint32),
					.structure_count = MAX_WORLD_PARTICLES,
					.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
					.debug_name		 = "particle_alive_b",
				});

			pfd.dead_indices.create(
				{
					.size		= sizeof(uint32) * MAX_WORLD_PARTICLES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "particle_dead_indices",
				},
				{
					.size			 = sizeof(uint32) * MAX_WORLD_PARTICLES,
					.structure_size	 = sizeof(uint32),
					.structure_count = MAX_WORLD_PARTICLES,
					.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
					.debug_name		 = "particle_dead_indices",
				});

			pfd.instance_data.create(
				{
					.size		= sizeof(particle_instance_data) * MAX_WORLD_PARTICLES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "particle_instances",
				},
				{
					.size			 = sizeof(particle_instance_data) * MAX_WORLD_PARTICLES,
					.structure_size	 = sizeof(particle_instance_data),
					.structure_count = MAX_WORLD_PARTICLES,
					.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
					.debug_name		 = "particle_instances",
				});

			pfd.counters.create(
				{
					.size		= sizeof(particle_counter),
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "particle_counters",
				},
				{
					.size			 = sizeof(particle_counter),
					.structure_size	 = sizeof(particle_counter),
					.structure_count = 1,
					.flags			 = resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
					.debug_name		 = "particle_counters",
				});
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
			pfd.emit_arguments.destroy();
			pfd.emit_counts.destroy();
			pfd.system_data.destroy();
			pfd.states.destroy();
			pfd.indirect_arguments.destroy();
			pfd.sim_count_indirect_arguments.destroy();
			pfd.alive_list_a.destroy();
			pfd.alive_list_b.destroy();
			pfd.dead_indices.destroy();
			pfd.counters.destroy();
			pfd.instance_data.destroy();
		}
	}

	void render_pass_particles::prepare(uint8 frame_index, proxy_manager& pm, const view& main_camera_view)
	{
		ZoneScoped;
		per_frame_data& pfd = _pfd[frame_index];

		auto&		 particles		= *pm.get_particle_emitters();
		const uint32 peak_particles = pm.get_peak_particle_emitters();

		uint32		num_emitters	= 0;
		const float particles_delta = static_cast<float>(frame_info::get_render_thread_time_milli());

		uint32			   emit_count = 0;
		bool			   emit_dead  = false;
		particle_emit_args args		  = {};

		for (render_proxy_particle_emitter& p : particles)
		{
			if (p.status != render_proxy_status::rps_active)
				continue;

			emit_count = 0;
			emit_dead  = false;

			if (!math::almost_equal(p.emit_props.emitter_lifetime, 0.0f))
			{
				if (p.current_life > p.emit_props.emitter_lifetime)
					emit_dead = true;
			}

			if (math::almost_equal(p.emit_props.wait_between_emits, 0.0f))
			{
				// emits constantly.
				emit_count = static_cast<uint32>(random::random_int(static_cast<int>(p.emit_props.min_particle_count), static_cast<int>(p.emit_props.max_particle_count)));
			}
			else
			{
				// emit in bursts
				if (p.current_life - p.last_emitted > p.emit_props.wait_between_emits)
				{
					p.last_emitted = p.current_life;
					emit_count	   = static_cast<uint32>(random::random_int(static_cast<int>(p.emit_props.min_particle_count), static_cast<int>(p.emit_props.max_particle_count)));
				}
			}

			if (!emit_dead)
				p.current_life += particles_delta;

			pfd.emit_counts.buffer_data(sizeof(uint32) * num_emitters, &emit_count, sizeof(uint32));

			const render_proxy_entity& e = pm.get_entity(p.entity);
			SFG_ASSERT(e.status == render_proxy_status::rps_active);
			const vector3 base_pos = e.model.get_translation();
			const vector3 min_pos  = base_pos + p.emit_props.min_pos_offset;
			const vector3 max_pos  = base_pos + p.emit_props.max_pos_offset;

			args = {
				.min_color = vector4(p.emit_props.min_color.x, p.emit_props.min_color.y, p.emit_props.min_color.z, p.emit_props.min_color.w),
				.max_color = vector4(p.emit_props.max_color.x, p.emit_props.max_color.y, p.emit_props.max_color.z, p.emit_props.max_color.w),
				.min_pos   = vector4(min_pos.x, min_pos.y, min_pos.z, p.emit_props.min_lifetime),
				.max_pos   = vector4(max_pos.x, max_pos.y, max_pos.z, p.emit_props.max_lifetime),
				.min_vel   = vector4(p.emit_props.min_vel_offset.x, p.emit_props.min_vel_offset.y, p.emit_props.min_vel_offset.z, DEG_2_RAD * p.emit_props.min_rotation_deg),
				.max_vel   = vector4(p.emit_props.max_vel_offset.x, p.emit_props.max_vel_offset.y, p.emit_props.max_vel_offset.z, DEG_2_RAD * p.emit_props.max_rotation_deg),
			};

			pfd.emit_arguments.buffer_data(sizeof(particle_emit_args) * num_emitters, &args, sizeof(particle_emit_args));

			num_emitters++;
		}

		gfx_backend* backend = gfx_backend::get();

		const gfx_id cmd_buffer = pfd.cmd_buffer_compute;

		backend->reset_command_buffer(cmd_buffer);

		if (num_emitters != 0)
		{
			static_vector<barrier, 2> barriers;
			barriers.push_back({
				.resource	 = pfd.emit_arguments.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_common,
				.to_states	 = resource_state::resource_state_copy_dest,
			});
			barriers.push_back({
				.resource	 = pfd.emit_counts.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_common,
				.to_states	 = resource_state::resource_state_copy_dest,
			});
			backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

			pfd.emit_arguments.copy_region(cmd_buffer, 0, num_emitters * sizeof(particle_emit_args));
			pfd.emit_counts.copy_region(cmd_buffer, 0, num_emitters * sizeof(uint32));

			barriers.resize(0);
			barriers.push_back({
				.resource	 = pfd.emit_arguments.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_non_ps_resource,
			});
			barriers.push_back({
				.resource	 = pfd.emit_counts.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_non_ps_resource,
			});
			backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});
		}

		const world_id main_cam	   = pm.get_main_camera();
		vector3		   cam_forward = vector3::zero;

		if (main_cam != NULL_WORLD_ID)
		{
			const render_proxy_camera& cam		  = pm.get_camera(main_cam);
			const render_proxy_entity& cam_entity = pm.get_entity(cam.entity);
			cam_forward							  = cam_entity.rotation.get_forward();
		}

		const ubo ubo_data = {
			.view_proj				  = main_camera_view.view_proj_matrix,
			.cam_pos_and_delta		  = vector4(main_camera_view.position.x, main_camera_view.position.y, main_camera_view.position.z, particles_delta),
			.cam_dir				  = vector4(cam_forward.x, cam_forward.y, cam_forward.z, 0.0f),
			.max_particles_per_system = MAX_WORLD_PARTICLES_PER_EMITTER,
			.frame_index			  = frame_index,
			.max_systems			  = MAX_WORLD_COMP_PARTICLE_EMITTERS,
			.num_systems			  = num_emitters,
		};

		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_particles::compute(uint8 frame_index)
	{
		ZoneScoped;
		gfx_backend*	backend	   = gfx_backend::get();
		per_frame_data& pfd		   = _pfd[frame_index];
		const gfx_id	cmd_buffer = pfd.cmd_buffer_compute;

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
