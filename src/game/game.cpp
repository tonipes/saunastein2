// Copyright (c) 2025 Inan Evin
#include "game/game.hpp"
#include "app/app.hpp"
#include "world/world.hpp"
#include "project/engine_data.hpp"

#include "resources/world_raw.hpp"
#include "resources/model.hpp"
#include "resources/animation.hpp"

#include "world/components/comp_model_instance.hpp"
#include "world/components/comp_mesh_instance.hpp"
#include "world/components/comp_animation_controller.hpp"
#include "math/math.hpp"
#include "math/random.hpp"

#ifdef SFG_TOOLMODE
#include "editor/editor.hpp"
#endif

#include <tracy/Tracy.hpp>

namespace SFG
{

	vector<pool_handle16> params;

	void game::init()
	{
		params.reserve(10000);

		world_raw raw = {};
		raw.load_from_file("assets/world/demo_world.stkworld", engine_data::get().get_working_dir().c_str());
		_app.get_world().create_from_loader(raw);

		_app.get_editor()->get_camera().activate();

		world&			   w  = _app.get_world();
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();
		resource_manager&  rm = w.get_resource_manager();

		const resource_handle boombox = rm.get_resource_handle_by_hash<model>(TO_SIDC("assets/character/character.stkmodel"));
		if (!rm.is_valid<model>(boombox))
			return;

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

		for (uint32 i = 0; i < 2000; i++)
		{
			const world_handle root = em.create_entity("root");
			em.set_entity_position(root, vector3::zero);
			em.set_entity_rotation(root, quat::identity);

			const world_handle	 model_inst_handle = cm.add_component<comp_model_instance>(root);
			comp_model_instance& mi				   = cm.get_component<comp_model_instance>(model_inst_handle);
			mi.instantiate_model_to_world(w, boombox);

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
				ag.add_transition(state1_handle, state2_handle, param_handle, 1.0f, 0.5f, animation_transition_compare::greater, 0);
				ag.add_transition(state2_handle, state1_handle, param_handle, 1.0f, 0.5f, animation_transition_compare::lesser, 0);
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