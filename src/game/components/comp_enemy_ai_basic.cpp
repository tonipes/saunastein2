#include "comp_enemy_ai_basic.hpp"
#include "reflection/reflection.hpp"
#include "world/components/comp_animation_controller.hpp"
#include "physics/physics_ray_collector.hpp"
#include "math/math.hpp"
#include "math/random.hpp"
#include "math/quat.hpp"

namespace SFG
{
	namespace
	{
		float random_range(float min_value, float max_value)
		{
			return min_value + (max_value - min_value) * random::random_01();
		}

		vector3 random_direction_xz()
		{
			const float angle = random_range(0.0f, MATH_TWO_PI);
			return vector3(math::cos(angle), 0.0f, math::sin(angle));
		}
	}

	void comp_enemy_ai_basic::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_enemy_ai_basic>::value, 0, "component");
		m.set_title("enemy_ai_basic");
		m.set_category("enemy");
		m.add_field<&comp_enemy_ai_basic::_movement_speed, comp_enemy_ai_basic>("movement_speed", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_attack_range, comp_enemy_ai_basic>("attack_range", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_damage, comp_enemy_ai_basic>("damage", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_aggro_range, comp_enemy_ai_basic>("aggro_range", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_health, comp_enemy_ai_basic>("health", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_idle_wait_min, comp_enemy_ai_basic>("idle_wait_min", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_idle_wait_max, comp_enemy_ai_basic>("idle_wait_max", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_wander_min, comp_enemy_ai_basic>("wander_min", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_wander_max, comp_enemy_ai_basic>("wander_max", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_wander_speed_min, comp_enemy_ai_basic>("wander_speed_min", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_wander_speed_max, comp_enemy_ai_basic>("wander_speed_max", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_attack_damage_delay, comp_enemy_ai_basic>("attack_damage_delay", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_attack_duration, comp_enemy_ai_basic>("attack_duration", reflected_field_type::rf_float, "");
	}

	void comp_enemy_ai_basic::begin_play(world& w)
	{
		_comp_manager	= &w.get_comp_manager();
		_entity_manager = &w.get_entity_manager();

		auto char_comp	 = _entity_manager->get_entity_component<comp_character_controller>(_header.entity);
		_char_controller = &_comp_manager->get_component<comp_character_controller>(char_comp);
		_anim_graph		 = &w.get_animation_graph();
		_physics_world	 = &w.get_physics_world();

		// Animation stuff
		world_handle anim_comp = {};
		_entity_manager->visit_children_deep(_header.entity, [&](world_handle e) {
			auto ac = _entity_manager->get_entity_component<comp_animation_controller>(e);
			if (!ac.is_null())
			{
				anim_comp = ac;
			}
		});
		if (!anim_comp.is_null())
		{
			auto anim_controller = _comp_manager->get_component<comp_animation_controller>(anim_comp);
			_anim_state_machine	 = anim_controller.get_runtime_machine();
			_walk_state			 = _anim_graph->get_state_handle(_anim_state_machine, "Walk");
			_idle_state			 = _anim_graph->get_state_handle(_anim_state_machine, "Idle");
			_attack_state		 = _anim_graph->get_state_handle(_anim_state_machine, "Attack");
			_death_state		 = _anim_graph->get_state_handle(_anim_state_machine, "Death");

			if (!_attack_state.is_null())
			{
				const float duration = _anim_graph->get_state(_attack_state).duration;
				if (duration > 0.0f)
					_attack_duration = duration;
			}
		}

		_state		  = enemy_state::idle_wait;
		_state_timer  = random_range(_idle_wait_min, _idle_wait_max);
		_wander_dir	  = random_direction_xz();
		_wander_speed = random_range(_wander_speed_min, _wander_speed_max);

		_player_entity = _entity_manager->find_entity("Player");
		if (!_player_entity.is_null())
			_player_stats = _entity_manager->get_entity_component<comp_player_stats>(_player_entity);
	}

	void comp_enemy_ai_basic::tick(comp_player_stats& player, float dt)
	{
		if (_char_controller == nullptr || _entity_manager == nullptr || _comp_manager == nullptr)
			return;

		if (_state == enemy_state::dead)
		{
			_char_controller->set_target_velocity(vector3::zero);
			return;
		}

		if (_health < 0.0f)
		{
			_state = enemy_state::dead;
			_char_controller->set_target_velocity(vector3::zero);
			if (!_anim_state_machine.is_null() && !_death_state.is_null())
				_anim_graph->set_machine_active_state(_anim_state_machine, _death_state);
			return;
		}

		if (_player_entity.is_null())
		{
			_player_entity = _entity_manager->find_entity("Player");
			if (!_player_entity.is_null())
				_player_stats = _entity_manager->get_entity_component<comp_player_stats>(_player_entity);
		}

		const vector3 self_pos = _entity_manager->get_entity_position_abs(_header.entity);

		if (!_has_aggro && !_player_entity.is_null())
		{
			const vector3 player_pos = _entity_manager->get_entity_position_abs(_player_entity);
			const float	  dist_sq	 = (player_pos - self_pos).magnitude_sqr();
			if (dist_sq <= _aggro_range * _aggro_range)
			{
				_has_aggro = true;
				_state	   = enemy_state::attack;
			}
		}
		const vector3 player_pos = _entity_manager->get_entity_position_abs(_player_entity);

		if (_has_aggro)
		{
			if (_player_entity.is_null())
				return;

			const vector3 to_player		  = player_pos - self_pos;
			const float	  dist_sq		  = to_player.magnitude_sqr();
			const float	  attack_range_sq = _attack_range * _attack_range;

			if (_is_attacking)
			{
				_attack_timer += dt;
				if (!_attack_damage_done && _attack_timer >= _attack_damage_delay)
				{
					if (!_player_stats.is_null())
					{
						comp_player_stats& stats = _comp_manager->get_component<comp_player_stats>(_player_stats);
						stats.take_damage(_damage);
					}
					_attack_damage_done = true;
				}

				if (_attack_timer >= _attack_duration)
				{
					_is_attacking		= false;
					_attack_timer		= 0.0f;
					_attack_damage_done = false;
				}

				_char_controller->set_target_velocity(vector3::zero);
				return;
			}

			if (dist_sq <= attack_range_sq)
			{
				_is_attacking		= true;
				_attack_timer		= 0.0f;
				_attack_damage_done = false;
				_char_controller->set_target_velocity(vector3::zero);
				if (!_anim_state_machine.is_null() && !_attack_state.is_null())
					_anim_graph->set_machine_active_state(_anim_state_machine, _attack_state);
				return;
			}

			vector3 move_dir = to_player;
			move_dir.y		 = 0.0f;
			if (!move_dir.is_zero())
			{
				move_dir.normalize();
				_face_dir = move_dir;
				_char_controller->set_target_velocity(move_dir * _movement_speed * 2.0f);
				set_animation_state_machine(false);
			}
			else
			{
				_char_controller->set_target_velocity(vector3::zero);
				set_animation_state_machine(true);
			}
			return;
		}

		_state_timer -= dt;
		if (_state == enemy_state::idle_wait)
		{
			_char_controller->set_target_velocity(vector3::zero);
			set_animation_state_machine(true);
			if (_state_timer <= 0.0f)
			{
				_state		  = enemy_state::idle_wander;
				_state_timer  = random_range(_wander_min, _wander_max);
				_wander_dir	  = random_direction_xz();
				_wander_speed = random_range(_wander_speed_min, _wander_speed_max);
			}
			return;
		}

		if (_state == enemy_state::idle_wander)
		{
			if (_wander_dir.is_zero())
				_wander_dir = random_direction_xz();

			_face_dir = _wander_dir;
			_char_controller->set_target_velocity(_wander_dir * (_movement_speed * _wander_speed));
			set_animation_state_machine(false);

			if (_state_timer <= 0.0f)
			{
				_state		 = enemy_state::idle_wait;
				_state_timer = random_range(_idle_wait_min, _idle_wait_max);
				_char_controller->set_target_velocity(vector3::zero);
				set_animation_state_machine(true);
			}
		}

		if (!_face_dir.is_zero())
		{
			world_handle mesh_entity = _entity_manager->get_child_by_index(_header.entity, 0);
			if (!mesh_entity.is_null())
			{
				const vector3 mesh_pos	  = _entity_manager->get_entity_position_abs(mesh_entity);
				vector3		  look_target = mesh_pos + _face_dir;
				look_target.y			  = mesh_pos.y;
				const quat look_rot		  = quat::look_at(mesh_pos, _has_aggro ? player_pos : look_target, vector3::up);
				_entity_manager->set_entity_rotation_abs(mesh_entity, look_rot);
			}
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

	void comp_enemy_ai_basic::take_damage(float damage)
	{
		_health -= damage;
	}
}
