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

#include "game/game.hpp"
#include "app/app.hpp"
#include "world/world.hpp"
#include "project/engine_data.hpp"

#include "resources/world_raw.hpp"
#include "resources/model.hpp"
#include "resources/animation.hpp"
#include "resources/material.hpp"
#include "resources/texture_sampler.hpp"
#include "resources/texture_sampler_raw.hpp"

#include "world/components/comp_model_instance.hpp"
#include "world/components/comp_mesh_instance.hpp"
#include "world/components/comp_animation_controller.hpp"
#include "world/components/comp_light.hpp"
#include "world/components/comp_ambient.hpp"
#include "world/components/comp_particle_emitter.hpp"
#include "math/math.hpp"
#include "math/random.hpp"

#include "gfx/common/common_material.hpp"

#ifdef SFG_TOOLMODE
#include "editor/editor.hpp"
#endif

#include <tracy/Tracy.hpp>

namespace SFG
{

	vector<pool_handle16> params;
	vector<world_handle>  ents;
	vector<vector3>		  randoms;

	void game::init()
	{
		params.reserve(10000);
		ents.reserve(10000);
		randoms.reserve(10000);

		world_raw raw = {};
		raw.load_from_file("assets/world/demo_world.stkworld", engine_data::get().get_working_dir().c_str());
		_app.get_world().create_from_loader(raw);

		_app.get_editor()->get_camera().activate();

		world&			   w  = _app.get_world();
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();
		resource_manager&  rm = w.get_resource_manager();

		const resource_handle particle_material = rm.get_resource_handle_by_hash<material>("assets/_defaults/materials/particle_additive.stkmat"_hs);

		const world_handle	   particle_handle		= em.create_entity("particle");
		const world_handle	   comp_particle_handle = cm.add_component<comp_particle_emitter>(particle_handle);
		comp_particle_emitter& emitter				= cm.get_component<comp_particle_emitter>(comp_particle_handle);
		emitter.set_emit_properties(w,
									{
										.emitter_lifetime		  = 0.0f,
										.wait_between_emits		  = 0.01f,
										.min_particle_count		  = 1,
										.max_particle_count		  = 1,
										.min_pos_offset			  = vector3(-0.15, 2, 0),
										.max_pos_offset			  = vector3(0.15, 1.75f, 0),
										.min_vel_offset			  = vector3(0, 2, 0),
										.max_vel_offset			  = vector3(0, 2, 0),
										.min_target_vel_offset	  = vector4(1, 2, 0, 1),
										.max_target_vel_offset	  = vector4(1, 2, 0, 1),
										.min_color				  = color(1, 0, 0, 0.25f),
										.max_color				  = color(1, 0, 1, 0.25f),
										.min_max_opacity_target	  = vector2(0, 0),
										.min_max_rotation_deg	  = vector2(0, 90),
										.min_max_angular_velocity = vector2(200, 200),
										.min_max_lifetime		  = vector2(3.0f, 3.0f),
										.min_max_size			  = vector2(0.5f, 0.5f),
										.min_max_target_size	  = vector2(0.4f, 0.4f),
									},
									particle_material);

		const resource_handle mdl = rm.get_resource_handle_by_hash<model>(TO_SIDC("assets/character/character.stkmodel"));
		if (!rm.is_valid<model>(mdl))
			return;

		const resource_handle mdl_cube = rm.get_resource_handle_by_hash<model>(TO_SIDC("assets/cube/cube.stkmodel"));
		if (!rm.is_valid<model>(mdl_cube))
			return;

		// anisotropic sampler
		{
			const resource_handle smp_handle = rm.add_resource<texture_sampler>(TO_SIDC("game:sampler_anisotropic"));
			texture_sampler&	  smp		 = rm.get_resource<texture_sampler>(smp_handle);

			texture_sampler_raw raw = {};
			raw.desc.min_lod		= 0.0f;
			raw.desc.max_lod		= 10.0f;
			raw.desc.anisotropy		= 8;
			raw.desc.address_u		= address_mode::repeat;
			raw.desc.address_v		= address_mode::repeat;
			raw.desc.address_w		= address_mode::repeat;
			raw.desc.flags			= sampler_flags::saf_min_anisotropic | sampler_flags::saf_mag_anisotropic | sampler_flags::saf_mip_linear;
			smp.create_from_loader(raw, w, smp_handle);

			const resource_handle mat_handle = rm.get_resource_handle_by_hash<material>(TO_SIDC("assets/ground/ground.stkmodel/GroundPlane"));
			material&			  mat		 = rm.get_resource<material>(mat_handle);
			mat.update_sampler(w, mat_handle, smp_handle);
		}

		// ground
		{
			const resource_handle ground_handle = rm.get_resource_handle_by_hash<model>(TO_SIDC("assets/ground/ground.stkmodel"));
			const world_handle	  entity		= em.create_entity("ground_plane");
			const world_handle	  inst			= cm.add_component<comp_model_instance>(entity);
			comp_model_instance&  mi			= cm.get_component<comp_model_instance>(inst);
			mi.instantiate_model_to_world(w, ground_handle);

			// monkey
			{
				const world_handle monkey = em.find_entity("Cube");
				const quat&		   rot	  = em.get_entity_rotation(monkey);
				const vector3	   dir	  = rot.get_forward();
				int				   a	  = 5;

				// em.set_entity_rotation(monkey, quat::look_at(em.get_entity_position(monkey), vector3(0, 0, 5), vector3::up));
				const vector3 dir2 = rot.get_forward();
				int			  b	   = 5;
			}

			// sun
			{
				const world_handle s_h = em.find_entity("Sun");
				const world_handle lh  = em.get_entity_component<comp_dir_light>(s_h);
				comp_dir_light&	   dl  = cm.get_component<comp_dir_light>(lh);
				// dl.set_values(w, {}, 0);

				em.set_entity_rotation(s_h, quat::from_euler(-30, 0, 0));
				dl.set_values(w, color(1, 1, 1, 1), 8);
				dl.set_shadow_values(w, 1, 3, vector2ui16(1024, 1024));
			}

			{
				// const world_handle p	 = em.find_entity("Spot");
				// const world_handle pl	 = em.get_entity_component<comp_spot_light>(p);
				// comp_spot_light&   light = cm.get_component<comp_spot_light>(pl);
				//// em.set_entity_position(p, vector3(0, 8, 0));
				// em.set_entity_rotation(p, quat::from_euler(-40, 0, 0));
				//
				//// light.set_values(w, color::white, 40, 60, DEG_2_RAD * 10.0f, DEG_2_RAD * 20.0f);
				// light.set_shadow_values(w, 1, 0.1f, vector2ui16(1024, 1024));
			}
		}

		// ambient
		{
			const world_handle entity = em.create_entity("ambient");
			const world_handle inst	  = cm.add_component<comp_ambient>(entity);
			comp_ambient&	   amb	  = cm.get_component<comp_ambient>(inst);
			amb.set_values(w, {0.05f, 0.05f, 0.05f, 0.0f});
		}

		constexpr float START = -20.0f;
		float			x_pos = START;
		float			z_pos = START;

		string_id anim_hashes[8] = {
			"assets/character/character.stkmodel/Walk_Loop"_hs,
			"assets/character/character.stkmodel/Interact"_hs,
			"assets/character/character.stkmodel/Roll"_hs,
			"assets/character/character.stkmodel/Punch_Jab"_hs,
			"assets/character/character.stkmodel/Jog_Fwd_Loop"_hs,
			"assets/character/character.stkmodel/Dance_Loop"_hs,
			"assets/character/character.stkmodel/Fixing_Kneeling"_hs,
			"assets/character/character.stkmodel/Idle_Talking_Loop"_hs,
		};

		for (uint32 i = 0; i < 10; i++)
		{
			const world_handle root = em.create_entity("root");
			em.set_entity_position(root, vector3::zero);
			em.set_entity_rotation(root, quat::identity);
			ents.push_back(root);

			randoms.push_back(vector3((random::random_01() * 2.f) - 1.f, 0.0f, (random::random_01() * 2.0f) - 1.0f) * 0.01f);

			const world_handle	 model_inst_handle = cm.add_component<comp_model_instance>(root);
			comp_model_instance& mi				   = cm.get_component<comp_model_instance>(model_inst_handle);
			mi.instantiate_model_to_world(w, mdl_cube);

			const float x = x_pos;
			const float z = z_pos;

			x_pos += 2.0f;
			if (x_pos > -START)
			{
				x_pos = START;
				z_pos += 2.0f;
			}

			em.set_entity_position(root, vector3(x, 0.0f, z));
		}

		for (uint32 i = 0; i < 10; i++)
		{
			const world_handle root = em.create_entity("root");
			em.set_entity_position(root, vector3::zero);
			em.set_entity_rotation(root, quat::identity);
			ents.push_back(root);

			randoms.push_back(vector3((random::random_01() * 2.f) - 1.f, 0.0f, (random::random_01() * 2.0f) - 1.0f) * 0.01f);

			const world_handle	 model_inst_handle = cm.add_component<comp_model_instance>(root);
			comp_model_instance& mi				   = cm.get_component<comp_model_instance>(model_inst_handle);
			mi.instantiate_model_to_world(w, mdl);

			const world_handle		   ces				= em.find_entity(root, "Mannequin");
			const world_handle		   comp_anim_handle = cm.add_component<comp_animation_controller>(ces);
			comp_animation_controller& comp_anim		= cm.get_component<comp_animation_controller>(comp_anim_handle);

			const world_handle		  comp_mesh_instance_handle = em.get_entity_component<comp_mesh_instance>(ces);
			const comp_mesh_instance& comp_mesh					= cm.get_component<comp_mesh_instance>(comp_mesh_instance_handle);
			const chunk_handle32	  skin_entities				= comp_mesh.get_skin_entities();

			animation_graph&	ag					 = w.get_animation_graph();
			const pool_handle16 state_machine_handle = comp_anim.get_state_machine();

			animation_state_machine& sm = ag.get_state_machine(state_machine_handle);
			sm.joint_entities			= comp_mesh.get_skin_entities();
			sm.joint_entities_count		= comp_mesh.get_skin_entities_count();

			const pool_handle16 param_handle = ag.add_parameter(state_machine_handle);
			params.push_back(param_handle);

			// states
			{
				const int			  rnd1		   = random::random_int(0, 7);
				const int			  rnd2		   = random::random_int(0, 7);
				const resource_handle anim_handle1 = rm.get_resource_handle_by_hash<animation>(anim_hashes[rnd1]);
				const resource_handle anim_handle2 = rm.get_resource_handle_by_hash<animation>(anim_hashes[rnd2]);

				// add states
				const pool_handle16 state1_handle = ag.add_state(state_machine_handle, {}, {}, {}, rm.get_resource<animation>(anim_handle1).get_duration(), animation_state_flags::animation_state_flags_is_looping);
				const pool_handle16 state2_handle = ag.add_state(state_machine_handle, {}, {}, {}, rm.get_resource<animation>(anim_handle2).get_duration(), animation_state_flags::animation_state_flags_is_looping);
				ag.set_machine_active_state(state_machine_handle, state1_handle);

				// add sample for each state
				ag.add_state_sample(state1_handle, anim_handle1, {});
				ag.add_state_sample(state2_handle, anim_handle2, {});

				// add transition between states
				ag.add_transition(state1_handle, state2_handle, param_handle, 0.0f, 0.5f, animation_transition_compare::greater, 0);
				ag.add_transition(state2_handle, state1_handle, param_handle, 0.0f, 0.5f, animation_transition_compare::lesser, 0);
			}

			const float x = x_pos;
			const float z = z_pos;

			x_pos += 1.0f;
			if (x_pos > -START)
			{
				x_pos = START;
				z_pos += 1.0f;
			}

			em.set_entity_position(root, vector3(x, 0.0f, z));
		}
	}

	void game::uninit()
	{
	}

	void game::pre_tick(float dt)
	{
		ZoneScoped;
	}

	static float ctr = 0.0f;
	static int	 val = 0.0f;

	void game::tick(float dt)
	{
		ZoneScoped;

		for (const pool_handle16 p : params)
		{
			animation_parameter& param = _world.get_animation_graph().get_parameter(p);
			param.value				   = val;
		}
		return;
		auto& em = _world.get_entity_manager();

		uint32 i = 0;
		for (auto e : ents)
		{
			auto& p = em.get_entity_position(e);
			em.set_entity_position(e, p + randoms[i]);
			i++;
		}

		ctr += dt;

		if (ctr > 5.0f)
		{
			ctr = 0.0f;
			val = 1 - val;
		}
	}

	void game::post_render()
	{
	}

	void game::on_window_event(const window_event& ev, window* wnd)
	{
	}
	void game::on_window_resize(const vector2ui16& size)
	{
	}
}