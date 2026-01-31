#pragma once
#include "world/components/common_comps.hpp"
#include "reflection/type_reflection.hpp"
#include "world/entity_manager.hpp"
#include "world/components/comp_character_controller.hpp"
#include "world/world.hpp"

namespace SFG
{
	class comp_enemy_ai_basic
	{
	public:
		static void reflect();

		void on_add(world& w);
		void on_remove(world& w);
		void begin_play(world& w);
		void tick(vector3 player_pos, float dt);

		inline const component_header& get_header() const
		{
			return _header;
		}

	private:
		template <typename T, int> friend class comp_cache;
		component_header _header = {};
		float					   _movement_speed = 1.0f;
		comp_character_controller* _char_controller;
		entity_manager*			   _entity_manager;
	};

	REFLECT_TYPE(comp_enemy_ai_basic);
}