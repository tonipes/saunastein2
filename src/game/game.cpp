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
#ifdef SFG_TOOLMODE
#include "editor/editor.hpp"
#endif

namespace SFG
{
	void game::init()
	{
		world_raw raw = {};
		raw.load_from_file("assets/world/demo_world.stkworld", engine_data::get().get_working_dir().c_str());
		_app.get_world().create_from_loader(raw);

		_app.get_editor()->get_camera().activate();

		world&			   w  = _app.get_world();
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();
		resource_manager&  rm = w.get_resource_manager();

		const resource_handle boombox = rm.get_resource_handle_by_hash<model>(TO_SIDC("assets/cesium_man/cesium_man.stkmodel"));
		if (!rm.is_valid<model>(boombox))
			return;

		constexpr float START = -12.5f;
		float			x_pos = START;
		float			z_pos = START;
		for (uint32 i = 0; i < 2048; i++)
		{
			const world_handle root = em.create_entity("boombox_root");
			em.set_entity_position(root, vector3::zero);
			em.set_entity_rotation(root, quat::identity);

			const world_handle	 model_inst_handle = cm.add_component<comp_model_instance>(root);
			comp_model_instance& mi				   = cm.get_component<comp_model_instance>(model_inst_handle);
			mi.instantiate_model_to_world(w, boombox);

			const world_handle		   ces				= em.find_entity(root, "Cesium_Man");
			const world_handle		   comp_anim_handle = cm.add_component<comp_animation_controller>(ces);
			comp_animation_controller& comp_anim		= cm.get_component<comp_animation_controller>(comp_anim_handle);

			const world_handle		  comp_mesh_instance_handle = em.get_entity_component<comp_mesh_instance>(ces);
			const comp_mesh_instance& comp_mesh					= cm.get_component<comp_mesh_instance>(comp_mesh_instance_handle);
			const chunk_handle32	  skin_entities				= comp_mesh.get_skin_entities();

			animation_graph&		 ag = w.get_animation_graph();
			animation_state_machine& sm = ag.get_state_machine(comp_anim.get_state_machine());
			sm.joint_entities			= comp_mesh.get_skin_entities();
			sm.joint_entities_count		= comp_mesh.get_skin_entities_count();

			const pool_handle16 state_handle = ag.add_state(comp_anim.get_state_machine());

			ag.set_machine_active_state(comp_anim.get_state_machine(), state_handle);

			animation_state& state = ag.get_state(state_handle);
			state.duration		   = 2.0f;
			state.flags.set(animation_state_flags::animation_state_flags_is_looping);

			const pool_handle16		sample = ag.add_state_sample(state_handle);
			animation_state_sample& smp	   = ag.get_sample(sample);
			smp.animation				   = rm.get_resource_handle_by_hash<animation>("assets/cesium_man/cesium_man.stkmodel/Anim_0.001"_hs);

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
	}

	void game::tick(float dt)
	{
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