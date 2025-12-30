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

	void render_pass_particles::init(gfx_id bind_layout, gfx_id bind_layout_compute)
	{
		gfx_backend* backend = gfx_backend::get();

		_indirect_sig_dispatch = backend->create_dispatch_indirect_signature(NULL_GFX_ID, sizeof(particle_sim_count_args));
		_indirect_sig_draw	   = backend->create_draw_indirect_signature(NULL_GFX_ID, sizeof(particle_indirect_args));

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
					.flags			 = resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
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
					.size		= sizeof(particle_counters),
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "particle_counters",
				},
				{
					.size			 = sizeof(particle_counters),
					.structure_size	 = sizeof(particle_counters),
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

		backend->destroy_indirect_signature(_indirect_sig_dispatch);
		backend->destroy_indirect_signature(_indirect_sig_draw);

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
				// emit at start.
				if (math::almost_equal(p.last_emitted, 0.0f))
					emit_count = static_cast<uint32>(random::random_int(static_cast<int>(p.emit_props.min_particle_count), static_cast<int>(p.emit_props.max_particle_count)));
				else if (p.current_life - p.last_emitted > p.emit_props.wait_between_emits)
				{
					// emit in bursts
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

		static_vector<barrier, 3> barriers;

		if (_buffers_init == 0)
		{
			const particle_sim_count_args dummy = {
				.group_sim_x   = 1,
				.group_sim_y   = 1,
				.group_sim_z   = 1,
				.group_count_x = 1,
				.group_count_y = 1,
				.group_count_z = 1,
			};

			pfd.sim_count_indirect_arguments.buffer_data(0, &dummy, sizeof(particle_sim_count_args));
			pfd.sim_count_indirect_arguments.copy(cmd_buffer);
		}

		if (num_emitters != 0)
		{
			pfd.emit_arguments.copy_region(cmd_buffer, 0, num_emitters * sizeof(particle_emit_args));
			pfd.emit_counts.copy_region(cmd_buffer, 0, num_emitters * sizeof(uint32));
		}

		if (!barriers.empty())
			backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

		barriers.resize(0);

		if (_buffers_init == 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_indirect_arg,
				.resource	 = pfd.sim_count_indirect_arguments.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});
		}

		if (num_emitters != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_non_ps_resource,
				.resource	 = pfd.emit_arguments.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_non_ps_resource,
				.resource	 = pfd.emit_counts.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});
		}

		if (!barriers.empty())
			backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

		_buffers_init = 1;

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
		_num_systems = num_emitters;
	}

	void render_pass_particles::compute(const compute_params& p)
	{
		ZoneScoped;
		gfx_backend*	backend	   = gfx_backend::get();
		per_frame_data& pfd		   = _pfd[p.frame_index];
		const gfx_id	cmd_buffer = pfd.cmd_buffer_compute;

		backend->cmd_bind_layout_compute(cmd_buffer, {.layout = p.global_layout_compute});
		backend->cmd_bind_group_compute(cmd_buffer, {.group = p.global_group});

		const gpu_index gpu_index_ubo					  = pfd.ubo.get_index();
		const gpu_index gpu_index_indirect_args			  = pfd.indirect_arguments.get_index();
		const gpu_index gpu_index_alive_list_a_srv		  = pfd.alive_list_a.get_index();
		const gpu_index gpu_index_alive_list_a_uav		  = pfd.alive_list_a.get_index_secondary();
		const gpu_index gpu_index_alive_list_b_srv		  = pfd.alive_list_b.get_index();
		const gpu_index gpu_index_alive_list_b_uav		  = pfd.alive_list_b.get_index_secondary();
		const gpu_index gpu_index_counters				  = pfd.counters.get_index();
		const gpu_index gpu_index_states				  = pfd.states.get_index();
		const gpu_index gpu_index_system_data_srv		  = pfd.system_data.get_index();
		const gpu_index gpu_index_system_data_uav		  = pfd.system_data.get_index_secondary();
		const gpu_index gpu_index_dead_list_srv			  = pfd.dead_indices.get_index();
		const gpu_index gpu_index_dead_list_uav			  = pfd.dead_indices.get_index_secondary();
		const gpu_index gpu_index_emit_counts			  = pfd.emit_counts.get_index();
		const gpu_index gpu_index_emit_args				  = pfd.emit_arguments.get_index();
		const gpu_index gpu_index_sim_count_indirect_args = pfd.sim_count_indirect_arguments.get_index();
		const gpu_index gpu_index_instances				  = pfd.instance_data.get_index();
		const gfx_id	hw_sim_count_indirect			  = pfd.sim_count_indirect_arguments.get_gpu();
		const gfx_id	hw_render_indirect				  = pfd.indirect_arguments.get_gpu();
		const gfx_id	hw_counters						  = pfd.counters.get_gpu();
		const gfx_id	hw_system_data					  = pfd.system_data.get_gpu();
		const gfx_id	hw_states						  = pfd.states.get_gpu();
		const gfx_id	hw_dead_indices					  = pfd.dead_indices.get_gpu();
		const gfx_id	hw_alive_list_a					  = pfd.alive_list_a.get_gpu();
		const gfx_id	hw_alive_list_b					  = pfd.alive_list_b.get_gpu();
		const gfx_id	hw_instances					  = pfd.instance_data.get_gpu();

		static_vector<barrier, 16> barriers;
		barriers.push_back({
			.from_states = resource_state::resource_state_indirect_arg,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_render_indirect,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_counters,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_common,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_system_data,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_states,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_dead_indices,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_alive_list_a,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_alive_list_b,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_common,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_instances,
			.flags		 = barrier_flags::baf_is_resource,
		});
		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

		// pass 0 - clear
		{
			const gpu_index constants[2] = {gpu_index_ubo, gpu_index_indirect_args};
			backend->cmd_bind_constants_compute(cmd_buffer, {.data = (uint8*)&constants, .offset = constant_index_rp_constant0, .count = 2, .param_index = rpi_constants});
		}
		backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = _shader_clear});
		backend->cmd_dispatch(cmd_buffer,
							  {
								  .group_size_x = (_num_systems / 64) + 1,
								  .group_size_y = 1,
								  .group_size_z = 1,
							  });

		// pass 1 - simulate
		{
			const gpu_index constants[7] = {
				gpu_index_ubo,
				gpu_index_alive_list_a_srv,
				gpu_index_alive_list_b_uav,
				gpu_index_counters,
				gpu_index_states,
				gpu_index_system_data_uav,
				gpu_index_dead_list_uav,
			};
			backend->cmd_bind_constants_compute(cmd_buffer, {.data = (uint8*)&constants, .offset = constant_index_rp_constant0, .count = 7, .param_index = rpi_constants});
		}

		backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = _shader_simulate});
		backend->cmd_execute_indirect(cmd_buffer,
									  {
										  .indirect_buffer		  = hw_sim_count_indirect,
										  .indirect_buffer_offset = 0,
										  .count				  = 1,
										  .indirect_signature	  = _indirect_sig_dispatch,
									  });

		// pass 1 simulate might write to below, so uav-uav
		barriers.resize(0);
		barriers.push_back({
			.resource = hw_counters,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});
		barriers.push_back({
			.resource = hw_states,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});
		barriers.push_back({
			.resource = hw_system_data,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_non_ps_resource,
			.resource	 = hw_dead_indices,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.resource = hw_alive_list_b,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});

		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

		// pass 2 - emit
		{
			const gpu_index constants[8] = {
				gpu_index_ubo,
				gpu_index_emit_counts,
				gpu_index_states,
				gpu_index_dead_list_srv,
				gpu_index_emit_args,
				gpu_index_system_data_uav,
				gpu_index_alive_list_b_uav,
				gpu_index_counters,
			};
			backend->cmd_bind_constants_compute(cmd_buffer, {.data = (uint8*)&constants, .offset = constant_index_rp_constant0, .count = 8, .param_index = rpi_constants});
		}

		backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = _shader_emit});
		backend->cmd_dispatch(cmd_buffer,
							  {
								  .group_size_x = 1,
								  .group_size_y = _num_systems == 0 ? 1 : _num_systems,
								  .group_size_z = 1,
							  });

		// we are done writing to counters for now, will be read as srv in in count passes.
		barriers.resize(0);
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_non_ps_resource,
			.resource	 = hw_counters,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_indirect_arg,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_sim_count_indirect,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_non_ps_resource,
			.resource	 = hw_alive_list_b,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_non_ps_resource,
			.resource	 = hw_states,
			.flags		 = barrier_flags::baf_is_resource,
		});
		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

		// pass 3 - write counts
		{
			const gpu_index constants[2] = {
				gpu_index_counters,
				gpu_index_sim_count_indirect_args,
			};
			backend->cmd_bind_constants_compute(cmd_buffer, {.data = (uint8*)&constants, .offset = constant_index_rp_constant0, .count = 2, .param_index = rpi_constants});
		}

		backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = _shader_write_count});
		backend->cmd_dispatch(cmd_buffer,
							  {
								  .group_size_x = 1,
								  .group_size_y = 1,
								  .group_size_z = 1,
							  });

		// wrote to sim and counts indirect arguments.
		barriers.resize(0);
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_indirect_arg,
			.resource	 = hw_sim_count_indirect,
			.flags		 = barrier_flags::baf_is_resource,
		});

		// was written by pass 0.
		barriers.push_back({
			.resource = hw_render_indirect,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});

		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

		// pass 4 - counts
		{
			const gpu_index constants[6] = {
				gpu_index_ubo,
				gpu_index_counters,
				gpu_index_alive_list_b_srv,
				gpu_index_states,
				gpu_index_indirect_args,
				gpu_index_instances,
			};
			backend->cmd_bind_constants_compute(cmd_buffer, {.data = (uint8*)&constants, .offset = constant_index_rp_constant0, .count = 6, .param_index = rpi_constants});
		}
		backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = _shader_count});
		backend->cmd_execute_indirect(cmd_buffer,
									  {
										  .indirect_buffer		  = hw_sim_count_indirect,
										  .indirect_buffer_offset = sizeof(uint32) * 3,
										  .count				  = 1,
										  .indirect_signature	  = _indirect_sig_dispatch,
									  });

		barriers.resize(0);
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_indirect_arg,
			.resource	 = hw_render_indirect,
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_indirect_arg,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_sim_count_indirect,
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_counters,
			.flags		 = barrier_flags::baf_is_resource,
		});

		// for graphics queue.
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_common,
			.resource	 = hw_instances,
			.flags		 = barrier_flags::baf_is_resource,
		});

		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

		// pass 5 - swap
		{
			const gpu_index constants[2] = {
				gpu_index_counters,
				gpu_index_sim_count_indirect_args,
			};
			backend->cmd_bind_constants_compute(cmd_buffer, {.data = (uint8*)&constants, .offset = constant_index_rp_constant0, .count = 2, .param_index = rpi_constants});
		}
		backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = _shader_swap});
		backend->cmd_dispatch(cmd_buffer,
							  {
								  .group_size_x = 1,
								  .group_size_y = 1,
								  .group_size_z = 1,
							  });

		barriers.resize(0);
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_indirect_arg,
			.resource	 = hw_sim_count_indirect,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_non_ps_resource,
			.resource	 = hw_counters,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_common,
			.resource	 = hw_instances,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_common,
			.resource	 = hw_system_data,
			.flags		 = barrier_flags::baf_is_resource,
		});
		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

		backend->close_command_buffer(cmd_buffer);
	}

	void render_pass_particles::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	backend	   = gfx_backend::get();
		per_frame_data& pfd		   = _pfd[p.frame_index];
		const gfx_id	cmd_buffer = pfd.cmd_buffer;

		backend->reset_command_buffer(cmd_buffer);

		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});

		const gpu_index gpu_index_ubo		  = pfd.ubo.get_index();
		const gpu_index gpu_index_system_data = pfd.system_data.get_index();
		const gpu_index gpu_index_instances	  = pfd.instance_data.get_index();
		const gfx_id	hw_system_data		  = pfd.system_data.get_gpu();
		const gfx_id	hw_instances		  = pfd.instance_data.get_gpu();
		const gfx_id	hw_render_indirect	  = pfd.indirect_arguments.get_gpu();

		static_vector<barrier, 4> barriers;
		barriers.push_back({
			.from_states = resource_state::resource_state_common,
			.to_states	 = resource_state::resource_state_non_ps_resource,
			.resource	 = hw_instances,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_common,
			.to_states	 = resource_state::resource_state_non_ps_resource,
			.resource	 = hw_system_data,
			.flags		 = barrier_flags::baf_is_resource,
		});
		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

		const uint32 constants[3] = {gpu_index_ubo, gpu_index_instances, gpu_index_system_data};
		backend->cmd_bind_constants(cmd_buffer, {.data = (uint8*)&constants, .offset = constant_index_rp_constant0, .count = 3, .param_index = rpi_constants});

		if (_num_systems != 0)
		{
			backend->cmd_execute_indirect(cmd_buffer,
										  {
											  .indirect_buffer		  = hw_render_indirect,
											  .indirect_buffer_offset = 0,
											  .count				  = static_cast<uint8>(_num_systems),
											  .indirect_signature	  = _indirect_sig_draw,
										  });
		}

		barriers.resize(0);
		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_common,
			.resource	 = hw_instances,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_common,
			.resource	 = hw_system_data,
			.flags		 = barrier_flags::baf_is_resource,
		});
		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<uint16>(barriers.size())});

		backend->close_command_buffer(cmd_buffer);
	}

}
