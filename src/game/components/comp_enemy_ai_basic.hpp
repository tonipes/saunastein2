#pragma once
#include "world/components/common_comps.hpp"
#include "reflection/type_reflection.hpp"
#include "world/entity_manager.hpp"
#include "world/components/comp_character_controller.hpp"
#include "world/world.hpp"
#include "game/components/comp_player_stats.hpp"
#include "world/components/comp_post_process.hpp"

namespace SFG
{
	class comp_enemy_ai_basic
	{
	public:
		enum class enemy_state
		{
			none,
			idle_wait,
			chase,
			attack,
			dead
		};

		static void reflect();

		void on_add(world& w);
		void on_remove(world& w);
		void begin_play(world& w);
		void tick(world& w, float dt);
		void set_animation_state_machine(bool idle);
		void take_damage(float damage);

		void tick_idle(world& w, float dt);
		void tick_attack(world& w, float dt);
		void tick_chase(world& w, float dt);
		void tick_death(world& w, float dt);
		void reset_runtime();

		void set_state(world& w, enemy_state state);

		inline const component_header& get_header() const
		{
			return _header;
		}

	private:
		template <typename T, int> friend class comp_cache;
		component_header _header = {};
		enemy_state		 _state	 = enemy_state::none;
		comp_post_process* _post_process = nullptr;

		float _min_chase_speed = 2.0f;
		float _max_chase_speed = 6.0f;
		float _min_chase_range = 5.0f;
		float _max_chase_range = 10.0f;

		float _chase_speed = 5.0f;
		float _chase_range = 10.0f;

		float _attack_damage_time_offset = 0.5f;
		float _attack_range				 = 3.0f;
		float _damage_range				 = 3.0f;
		float _min_damage				 = 0.0f;
		float _max_damage				 = 0.0f;
		float _damage					 = 0.0f;
		float _attack_start_time		 = 0.0f;
		float _attack_anim_duration		 = 0.0f;
		bool  _attack_done_once			 = false;

		world_handle  _player					 = {};
		world_handle  _mesh_entity				 = {};
		world_handle  _comp_player_handle		 = {};
		world_handle  _comp_animator			 = {};
		world_handle  _comp_character_controller = {};
		pool_handle16 _anim_idle				 = {};
		pool_handle16 _anim_attack				 = {};
		pool_handle16 _anim_death				 = {};
		pool_handle16 _anim_walk				 = {};
		pool_handle16 _state_machine			 = {};
	};

	REFLECT_TYPE(comp_enemy_ai_basic);
}
