/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "common/string_id.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "resources/common_resources.hpp"
#include "world/world_constants.hpp"
#include "physics/physics_contact_listener.hpp"
#include "physics/physics_character_contact_listener.hpp"

namespace SFG
{
	struct managed_entity_params {
		float		 max_lifetime;
		float		 speed;
		bool destroy_on_collision;
	};

	static inline managed_entity_params bullet_params = {
		.max_lifetime = 10.0f,
		.speed = 10.0f,
		.destroy_on_collision = false
	};

	struct door
	{
		// world_handle door_handle;
		world_handle door_root_handle;
		float		 t;
		float		 open_angle;
		bool		 is_opened;
		float		 auto_open_distance;
		float		 direction;
	};

	struct managed_entity
	{
		world_handle handle;
		float		 t;
		managed_entity_params params;
		bool marked_for_removal;
	};


	class world;
	class app;
	class window;
	struct window_event;
	struct vector2ui16;
	class quat;

	class gameplay : public physics_contact_listener, public physics_character_contact_listener
	{
	public:
		gameplay(app& app) : _app(app) {};

		static gameplay& get()
		{
			return *_inst;
		}

		void init();
		void uninit();

		void on_world_begin(world& w);
		void on_world_end(world& w);
		void on_debug_tick(world& w, float dt, const vector2ui16& game_res);
		void on_world_tick(world& w, float dt, const vector2ui16& game_res);
		void on_window_event(const window_event& ev, window* wnd);
		void on_contact_begin(world_handle e1, world_handle e2, const vector3& p1, const vector3& p2) override;
		void on_contact(world_handle e1, world_handle e2, const vector3& p1, const vector3& p2) override;
		void on_contact_end(world_handle e1, world_handle e2) override;
		void on_character_contact_begin(world_handle character, world_handle other, const vector3& position, const vector3& normal) override;
		void on_character_contact(world_handle character, world_handle other, const vector3& position, const vector3& normal) override;
		void on_character_contact_end(world_handle character, world_handle other) override;
		world_handle spawn_managed_entity(string_id resource, vector3 position, quat direction, const managed_entity_params& params);

	private:
		void tick_player(float dt);
		void tick_player_debug(float dt);
		void begin_player();
		void tick_enemies(float dt);
		void begin_enemies();

		void tick_doors(float dt);
		void begin_doors();

		void begin_managed_entities();
		void tick_managed_entities(float dt);

		void check_managed_entities_collision(world_handle e1, world_handle e2);

	private:
		app&		 _app;
		world_handle _player_entity = {};
		vector<door> _doors			= {};
		vector<managed_entity> _managed_entities = {};
		bool		 _player_initialized = false;	
		static gameplay*	   _inst;
	};
}
