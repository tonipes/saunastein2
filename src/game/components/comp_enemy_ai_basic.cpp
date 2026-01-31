#include "comp_enemy_ai_basic.hpp"
#include "reflection/reflection.hpp"
#include "world/components/comp_animation_controller.hpp"
#include "physics/physics_ray_collector.hpp"

namespace SFG
{
	void comp_enemy_ai_basic::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_enemy_ai_basic>::value, 0, "component");
		m.set_title("enemy_ai_basic");
		m.set_category("enemy");
		m.add_field<&comp_enemy_ai_basic::_movement_speed, comp_enemy_ai_basic>("movement_speed", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_attack_range, comp_enemy_ai_basic>("attack_range", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_damage, comp_enemy_ai_basic>("damage", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_attack_cooldown, comp_enemy_ai_basic>("attack_cooldown", reflected_field_type::rf_float, "");
	}

	void comp_enemy_ai_basic::begin_play(world& w)
	{
		_comp_manager		 = &w.get_comp_manager();
		_entity_manager		 = &w.get_entity_manager();
		auto  char_comp		 = _entity_manager->get_entity_component<comp_character_controller>(_header.entity);
		_char_controller	 = &_comp_manager->get_component<comp_character_controller>(char_comp);
		_anim_graph			 = &w.get_animation_graph();
		_physics_world		 = &w.get_physics_world();

		// Animation stuff
		world_handle		 anim_comp = {};
		_entity_manager->visit_children_deep(_header.entity, [&](world_handle e) {
			auto ac = _entity_manager->get_entity_component<comp_animation_controller>(e);
			if (!ac.is_null())
			{
				anim_comp = ac;
			}
		});
		if (anim_comp.is_null())
			return;
		auto anim_controller = _comp_manager->get_component<comp_animation_controller>(anim_comp);
		_anim_state_machine	 = anim_controller.get_runtime_machine();
		_walk_state			 = _anim_graph->get_state_handle(_anim_state_machine, "Walk");
		_idle_state			 = _anim_graph->get_state_handle(_anim_state_machine, "Idle");
	}

	void comp_enemy_ai_basic::tick(comp_player_stats& player, float dt)
	{
		_lifetime += dt;
		auto player_pos = _entity_manager->get_entity_position(player.get_header().entity);
		auto pos	 = _entity_manager->get_entity_position(_header.entity);
		player_pos.y = pos.y;
		auto diff	 = player_pos - pos;

		// Attack check
		if (vector3::distance_sqr(player_pos, pos) < _attack_range * _attack_range)
		{
			if (_lifetime - _last_attack_time > _attack_cooldown)
			{
				// Attack player
				_last_attack_time = _lifetime;
				player.take_damage(_damage);
			}
		}

		if ((int)_lifetime % 2 || diff.is_zero())
		{
			_char_controller->set_target_velocity(vector3::zero);
			set_animation_state_machine(true);
		}
		else
		{
			diff			  = diff.normalized();
			const vector3 vel = diff * _movement_speed;
			_char_controller->set_target_velocity(vel);
			set_animation_state_machine(false);
			_entity_manager->set_entity_rotation(_header.entity, quat::look_at(pos, player_pos, vector3::up));
		}
	}

	void comp_enemy_ai_basic::set_animation_state_machine(bool idle)
	{
		if (!_anim_state_machine.is_null() && idle != _is_idle)
			_anim_graph->set_machine_active_state(_anim_state_machine, idle ? _idle_state : _walk_state);
		_is_idle = idle;
	}

	void comp_enemy_ai_basic::on_add(world& w)
	{
	}

	void comp_enemy_ai_basic::on_remove(world& w)
	{
	}
}