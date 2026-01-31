#include "comp_enemy_ai_basic.hpp"
#include "reflection/reflection.hpp"

namespace SFG
{
	void comp_enemy_ai_basic::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_enemy_ai_basic>::value, 0, "component");
		m.set_title("enemy_ai_basic");
		m.set_category("enemy");
		m.add_field<&comp_enemy_ai_basic::_movement_speed, comp_enemy_ai_basic>("movement_speed", reflected_field_type::rf_float, "");
	}

	void comp_enemy_ai_basic::begin_play(world& w)
	{
		auto componentHandle = w.get_entity_manager().get_entity_component<comp_character_controller>(_header.entity);
		_char_controller = &w.get_comp_manager().get_component<comp_character_controller>(componentHandle);
		_entity_manager	 = &w.get_entity_manager();
	}

	void comp_enemy_ai_basic::tick(vector3 player_pos, float dt)
	{
		auto pos  = _entity_manager->get_entity_position(_header.entity);
		auto diff = player_pos - pos;
		diff.y	  = 0.0f;
		diff	  = diff.normalized();
		const vector3 vel = diff * _movement_speed;
		_char_controller->set_target_velocity(vel);
	}

	void comp_enemy_ai_basic::on_add(world& w)
	{
	}

	void comp_enemy_ai_basic::on_remove(world& w)
	{
	}
}