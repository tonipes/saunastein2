#pragma once
#include "world/components/common_comps.hpp"
#include "reflection/type_reflection.hpp"
#include "world/entity_manager.hpp"
#include "world/components/comp_character_controller.hpp"
#include "world/world.hpp"
#include "game/components/comp_player_stats.hpp"

namespace SFG
{
	class comp_enemy_ai_basic
	{
	public:
		static void reflect();

		void on_add(world& w);
		void on_remove(world& w);
		void begin_play(world& w);
		void tick(comp_player_stats& player, float dt);
		void set_animation_state_machine(bool idle);
		void take_damage(float damage);

		inline const component_header& get_header() const
		{
			return _header;
		}

	private:
		enum class enemy_state
		{
			idle_wait,
			idle_wander,
			attack,
			dead,
		};

		template <typename T, int> friend class comp_cache;
		component_header		   _header		   = {};
		float					   _movement_speed = 1.0f;
		float					   _attack_range   = 1.5f;
		float					   _damage		   = 10.0f;
		float					   _aggro_range	   = 6.0f;
		float					   _health		   = 30.0f;
		float					   _idle_wait_min  = 0.75f;
		float					   _idle_wait_max  = 2.25f;
		float					   _wander_min	   = 1.0f;
		float					   _wander_max	   = 3.0f;
		float					   _attack_damage_delay = 0.5f;
		float					   _attack_duration	 = 1.0f;
		float					   _wander_speed_min	 = 0.6f;
		float					   _wander_speed_max	 = 1.0f;
		comp_character_controller* _char_controller;
		entity_manager*			   _entity_manager;
		component_manager*		   _comp_manager;
		bool					   _is_idle = false;
		physics_world*			   _physics_world;
		enemy_state				   _state			= enemy_state::idle_wait;
		float					   _state_timer		= 0.0f;
		vector3					   _wander_dir		= vector3::zero;
		float					   _wander_speed	= 0.0f;
		vector3					   _face_dir		= vector3::forward;
		bool					   _has_aggro		= false;
		bool					   _is_attacking	= false;
		bool					   _attack_damage_done = false;
		float					   _attack_timer	= 0.0f;
		world_handle			   _player_entity	= {};
		world_handle			   _player_stats		= {};

		// Animation stuff
		animation_graph* _anim_graph;
		resource_handle	 _anim_state_machine = {};
		pool_handle16	 _idle_state;
		pool_handle16	 _walk_state;
		pool_handle16	 _attack_state;
		pool_handle16	 _death_state;
	};

	REFLECT_TYPE(comp_enemy_ai_basic);
}
