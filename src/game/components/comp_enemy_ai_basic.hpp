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

		inline const component_header& get_header() const
		{
			return _header;
		}

	private:
		template <typename T, int> friend class comp_cache;
		component_header		   _header		   = {};
		float					   _movement_speed = 1.0f;
		float					   _attack_range   = 1.5f;
		float					   _damage		   = 10.0f;
		float					   _attack_cooldown = 10.0f;
		comp_character_controller* _char_controller;
		entity_manager*			   _entity_manager;
		component_manager*		   _comp_manager;
		float					   _lifetime = 0.0f;
		bool					   _is_idle	  = false;
		physics_world*			   _physics_world;
		float					   _last_attack_time = -9999.0f;

		// Animation stuff
		animation_graph* _anim_graph;
		resource_handle	 _anim_state_machine = {};
		pool_handle16	 _idle_state;
		pool_handle16	 _walk_state;
	};

	REFLECT_TYPE(comp_enemy_ai_basic);
}