// Copyright (c) 2025 Inan Evin
#include "game/game.hpp"
#include "app/app.hpp"
#include "world/world.hpp"
#include "project/engine_data.hpp"

#include "resources/world_raw.hpp"
#include "resources/model.hpp"

#include "world/components/comp_model_instance.hpp"

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

		const world_handle root = em.create_entity("boombox_root");
		em.set_entity_position(root, vector3::zero);
		em.set_entity_rotation(root, quat::identity);

		const resource_handle boombox = rm.get_resource_handle_by_hash<model>(TO_SIDC("assets/cesium_man/cesium_man.stkmodel"));
		if (!rm.is_valid<model>(boombox))
			return;

		const world_handle	 model_inst_handle = cm.add_component<comp_model_instance>(root);
		comp_model_instance& mi				   = cm.get_component<comp_model_instance>(model_inst_handle);
		mi.instantiate_model_to_world(w, boombox);
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