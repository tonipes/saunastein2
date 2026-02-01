#include "comp_enemy_ai_basic.hpp"
#include "comp_player.hpp"
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

	bool _inited = false;

	void comp_enemy_ai_basic::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_enemy_ai_basic>::value, 0, "component");
		m.set_title("enemy_ai_basic");
		m.set_category("enemy");
		m.add_field<&comp_enemy_ai_basic::_min_chase_speed, comp_enemy_ai_basic>("min_chase_speed", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_max_chase_speed, comp_enemy_ai_basic>("max_chase_speed", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_min_chase_range, comp_enemy_ai_basic>("min_chase_range", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_max_chase_range, comp_enemy_ai_basic>("max_chase_range", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_attack_range, comp_enemy_ai_basic>("attack_range", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_damage_range, comp_enemy_ai_basic>("damage_range", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_min_damage, comp_enemy_ai_basic>("min_damage", reflected_field_type::rf_float, "");
		m.add_field<&comp_enemy_ai_basic::_max_damage, comp_enemy_ai_basic>("max_damage", reflected_field_type::rf_float, "");

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) { comp_enemy_ai_basic* c = static_cast<comp_enemy_ai_basic*>(params.object_ptr); });
		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) { comp_enemy_ai_basic* c = static_cast<comp_enemy_ai_basic*>(obj); });
	}

	void comp_enemy_ai_basic::reset_runtime()
	{
		_attack_done_once  = false;
		_inited			   = false;
		_attack_start_time = 0.0f;
		_chase_range	   = _min_chase_range + random::random_01() * (_max_chase_range - _min_chase_range);
		_chase_speed	   = _min_chase_speed + random::random_01() * (_max_chase_speed - _min_chase_speed);
		_damage			   = _min_damage + random::random_01() * (_max_damage - _min_damage);
	}

	void comp_enemy_ai_basic::set_state(world& w, enemy_state state)
	{
		_state = state;

		if (state == enemy_state::idle_wait)
		{
			animation_graph& ag = w.get_animation_graph();
			ag.set_machine_active_state(_state_machine, _anim_idle);
		}
		else if (state == enemy_state::chase)
		{
			animation_graph& ag = w.get_animation_graph();
			ag.set_machine_active_state(_state_machine, _anim_walk);
		}
		else if (state == enemy_state::attack)
		{
			animation_graph& ag = w.get_animation_graph();
			ag.set_machine_active_state(_state_machine, _anim_attack);
			_attack_start_time = w.get_time_manager().get_elapsed_real_time();
			_attack_done_once  = false;
		}
		else if (state == enemy_state::dead)
		{
			animation_graph& ag = w.get_animation_graph();
			ag.set_machine_active_state(_state_machine, _anim_death);
		}
	}

	void comp_enemy_ai_basic::begin_play(world& w)
	{
		reset_runtime();

		entity_manager& em = w.get_entity_manager();
		for (auto& pp : w.get_comp_manager().underlying_pool<comp_cache<comp_post_process, MAX_WORLD_COMP_POST_PROCESS>, comp_post_process>())
		{
			_post_process = &pp;
			break;
		}

		_comp_animator = {};

		bool ok = false;
		em.visit_children_deep(_header.entity, [&](world_handle e) {
			if (ok)
				return;

			auto ac = em.get_entity_component<comp_animation_controller>(e);
			if (!ac.is_null())
			{
				_comp_animator = ac;
				ok			   = true;
				return;
			}
		});
		if (_comp_animator.is_null())
			return;

		_player = em.find_entity("Player");
		if (_player.is_null())
			return;

		_comp_player_handle = em.get_entity_component<comp_player>(_player);
		if (_comp_player_handle.is_null())
			return;

		_comp_character_controller = em.get_entity_component<comp_character_controller>(_header.entity);
		if (_comp_character_controller.is_null())
			return;

		component_manager&		   cm	= w.get_comp_manager();
		comp_animation_controller& anim = cm.get_component<comp_animation_controller>(_comp_animator);
		_state_machine					= anim.get_runtime_machine();

		if (_state_machine.is_null())
			return;

		animation_graph& anim_graph = w.get_animation_graph();
		_anim_walk					= anim_graph.get_state_handle(_state_machine, "Walk");
		_anim_idle					= anim_graph.get_state_handle(_state_machine, "Idle");
		_anim_attack				= anim_graph.get_state_handle(_state_machine, "Attack");
		_anim_death					= anim_graph.get_state_handle(_state_machine, "Death");
		_attack_anim_duration		= anim_graph.get_state(_anim_attack).duration / anim_graph.get_state(_anim_attack).speed;

		_mesh_entity = em.get_entity_family(_header.entity).first_child;

		set_state(w, enemy_state::idle_wait);
		_inited = true;
	}

	void comp_enemy_ai_basic::tick(world& w, float dt)
	{
		if (!_inited)
			return;

		switch (_state)
		{
		case enemy_state::idle_wait:
			tick_idle(w, dt);
			break;
		case enemy_state::chase:
			tick_chase(w, dt);
			break;
		case enemy_state::attack:
			tick_attack(w, dt);
			break;
		case enemy_state::dead:
			tick_death(w, dt);
			break;
		}
	}

	void comp_enemy_ai_basic::set_animation_state_machine(bool idle)
	{
	}

	void comp_enemy_ai_basic::on_add(world& w)
	{
	}

	void comp_enemy_ai_basic::on_remove(world& w)
	{
	}

	void comp_enemy_ai_basic::tick_idle(world& w, float dt)
	{
		entity_manager& em		   = w.get_entity_manager();
		const vector3&	player_pos = em.get_entity_position(_player);
		const vector3&	own_pos	   = em.get_entity_position(_header.entity);
		const float		dist	   = (player_pos - own_pos).magnitude_sqr();

		if (dist <= _chase_range * _chase_range)
		{
			set_state(w, enemy_state::chase);
		}
	}

	void comp_enemy_ai_basic::tick_chase(world& w, float dt)
	{
		entity_manager& em		   = w.get_entity_manager();
		const vector3&	player_pos = em.get_entity_position(_player);
		const vector3&	own_pos	   = em.get_entity_position(_header.entity);
		const vector3	to_player  = player_pos - own_pos;
		vector3			velocity   = to_player.normalized();
		velocity.y				   = 0.0f;

		component_manager& cm	  = w.get_comp_manager();
		comp_player&	   player = cm.get_component<comp_player>(_comp_player_handle);

		comp_character_controller& own_char = cm.get_component<comp_character_controller>(_comp_character_controller);
		own_char.set_target_velocity(velocity * _chase_speed);

		em.set_entity_rotation_abs(_mesh_entity, quat::look_at(own_pos, player_pos, vector3::up));

		if (to_player.magnitude_sqr() <= _attack_range * _attack_range)
		{
			set_state(w, enemy_state::attack);
		}
	}

	void comp_enemy_ai_basic::tick_attack(world& w, float dt)
	{
		component_manager& cm = w.get_comp_manager();

		comp_character_controller& own_char = cm.get_component<comp_character_controller>(_comp_character_controller);
		own_char.set_target_velocity(vector3::zero);

		entity_manager& em = w.get_entity_manager();

		const float elapsed = w.get_time_manager().get_elapsed_real_time();

		const vector3& player_pos = em.get_entity_position(_player);
		const vector3& own_pos	  = em.get_entity_position(_header.entity);
		const vector3  to_player  = player_pos - own_pos;
		const float	   dist		  = to_player.magnitude_sqr();

		if (!_attack_done_once && elapsed > _attack_start_time + _attack_damage_time_offset)
		{
			if (dist <= _damage_range * _damage_range)
			{
				// damage here.
				comp_player&	   cp	 = cm.get_component<comp_player>(_comp_player_handle);
				comp_player_stats& stats = cm.get_component<comp_player_stats>(cp.get_stats());
				stats.take_damage(_damage);
				_attack_done_once = true;
				_post_process->set_damage_time(w, elapsed);
			}
		}

		if (elapsed > _attack_start_time + _attack_anim_duration)
		{
			if (dist <= _attack_range * _attack_range)
			{
				set_state(w, enemy_state::attack);
			}
			else
				set_state(w, enemy_state::chase);
		}
	}

	void comp_enemy_ai_basic::tick_death(world& w, float dt)
	{
	}

	void comp_enemy_ai_basic::take_damage(float damage)
	{
	}
}
