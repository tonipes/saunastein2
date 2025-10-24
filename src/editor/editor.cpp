// Copyright (c) 2025 Inan Evin

#include "editor.hpp"

#include "app/game_app.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "world/traits/trait_camera.hpp"
#include "world/traits/trait_model_instance.hpp"
#include "world/traits/trait_ambient.hpp"
#include "resources/model.hpp"
#include "platform/window.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "common/string_id.hpp"

namespace SFG
{
	editor::editor(game_app& game) : _game(game)
	{
	}

	editor::~editor() = default;

	void editor::on_init()
	{
		create_default_camera();
		create_demo_content();
	}

	void editor::on_uninit()
	{
		destroy_demo_content();
	}

	void editor::on_tick(float dt_seconds)
	{
		if (_camera_controller.is_active())
			_camera_controller.tick(dt_seconds);
	}

	void editor::on_post_tick(double)
	{
	}

	void editor::on_pre_render(const vector2ui16&)
	{
	}

	void editor::on_render()
	{
	}

	bool editor::on_window_event(const window_event& ev)
	{

		if (_camera_controller.is_active())
			_camera_controller.on_window_event(ev);

		return true;
	}

	void editor::create_default_camera()
	{
		world* world_ptr = _game.get_world();
		if (!world_ptr)
			return;

		world&			w  = *world_ptr;
		entity_manager& em = w.get_entity_manager();
		trait_manager&	tm = w.get_trait_manager();
		_camera_entity	   = em.create_entity("editor_camera");
		_camera_trait	   = tm.add_trait<trait_camera>(_camera_entity);

		trait_camera& cam_trait = tm.get_trait<trait_camera>(_camera_trait);
		cam_trait.set_values(w, 0.01f, 500.0f, 60.0f);
		cam_trait.set_main(w);

		em.set_entity_position(_camera_entity, vector3(0.0f, 1.5f, 10.0f));
		em.set_entity_rotation(_camera_entity, quat::identity);

		window* main_window = _game.get_main_window();
		_camera_controller.init(w, _camera_entity, main_window);
	}

	void editor::create_demo_content()
	{
		world* world_ptr = _game.get_world();
		if (!world_ptr)
			return;

		world&				  w		  = *world_ptr;
		resource_manager&	  rm	  = w.get_resource_manager();
		const resource_handle boombox = rm.get_resource_handle_by_hash<model>(TO_SIDC("assets/test_scene/test_scene.stkmodel"));

		if (!rm.is_valid<model>(boombox))
			return;
		entity_manager& em = w.get_entity_manager();
		trait_manager&	tm = w.get_trait_manager();

		_demo_model_root = em.create_entity("boombox_root");
		em.set_entity_position(_demo_model_root, vector3::zero);
		em.set_entity_rotation(_demo_model_root, quat::identity);

		const world_handle	  model_inst_handle = tm.add_trait<trait_model_instance>(_demo_model_root);
		trait_model_instance& mi				= tm.get_trait<trait_model_instance>(model_inst_handle);
		mi.set_model(boombox);
		mi.instantiate_model_to_world(w, boombox);

		_ambient_entity			 = em.create_entity("ambient");
		_ambient_trait			 = tm.add_trait<trait_ambient>(_ambient_entity);
		trait_ambient& trait_amb = tm.get_trait<trait_ambient>(_ambient_trait);
		trait_amb.set_values(w, color(0.1f, 0.1f, 0.1f));
	}

	void editor::destroy_demo_content()
	{
		world* world_ptr = _game.get_world();
		if (!world_ptr)
			return;

		world&			w  = *world_ptr;
		entity_manager& em = w.get_entity_manager();
		trait_manager&	tm = w.get_trait_manager();

		if (!_ambient_entity.is_null())
		{
			tm.remove_trait<trait_ambient>(_ambient_entity, _ambient_trait);
			em.destroy_entity(_ambient_entity);
			_ambient_entity = {};
			_ambient_trait	= {};
		}

		_camera_controller.uninit();

		if (!_camera_trait.is_null())
		{
			tm.remove_trait<trait_camera>(_camera_entity, _camera_trait);
			_camera_trait = {};
		}

		if (!_camera_entity.is_null())
		{
			em.destroy_entity(_camera_entity);
			_camera_entity = {};
		}

		if (!_demo_model_root.is_null())
		{
			em.destroy_entity(_demo_model_root);
			_demo_model_root = {};
		}
	}
}
