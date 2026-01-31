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

#include "gameplay.hpp"
#include "app/app.hpp"
#include "world/world.hpp"
#include "game/components/comp_player.hpp"
#include "game/components/comp_player_stats.hpp"
#include <world/components/comp_physics.hpp>
#include "game/components/comp_enemy_ai_basic.hpp"
#include <physics/physics_contact_listener.hpp>
#include "platform/window_common.hpp"
#include "input/input_mappings.hpp"

namespace SFG
{

#define MAX_PLAYERS 1

	void gameplay::init()
	{
		_app.get_world().get_comp_manager().register_cache<comp_player, MAX_PLAYERS>();
		_app.get_world().get_comp_manager().register_cache<comp_player_stats, MAX_PLAYERS>();
	}

	void gameplay::uninit()
	{
	}

	void gameplay::on_world_begin(world& w)
	{
		w.get_physics_world().set_contact_listener(this);
		w.get_physics_world().set_character_contact_listener(this);
		begin_player();
		begin_doors();
		begin_managed_entities();
	}

	void gameplay::on_world_end(world& w)
	{
		w.get_physics_world().set_contact_listener(nullptr);
		w.get_physics_world().set_character_contact_listener(nullptr);
		_doors.clear();
	}

	void gameplay::on_debug_tick(world& w, float dt, const vector2ui16& game_res)
	{
		tick_player_debug(dt);
	}

	void gameplay::on_world_tick(world& w, float dt, const vector2ui16& game_res)
	{
		tick_player(dt);
		tick_doors(dt);
		tick_managed_entities(dt);
	}

	void gameplay::on_window_event(const window_event& ev, window* wnd)
	{
		world&			   w	   = _app.get_world();
		component_manager& cm	   = w.get_comp_manager();
		auto&			   players = cm.underlying_pool<comp_cache<comp_player, MAX_PLAYERS>, comp_player>();
		for (comp_player& p : players)
			p.on_window_event(ev);


		switch (ev.type)
		{
			case window_event_type::key: {
				if (ev.button == input_code::key_space && ev.sub_type == window_event_sub_type::press)
				{
					SFG_TRACE("SPACE PRESSED");
				}
			}
			default:
				break;
		}
	}

	void gameplay::on_contact_begin(world_handle e1, world_handle e2, const vector3& p1, const vector3& p2)
	{
		check_managed_entities_collision(e1, e2);
	}

	void gameplay::on_contact(world_handle e1, world_handle e2, const vector3& p1, const vector3& p2)
	{
	}

	void gameplay::on_contact_end(world_handle e1, world_handle e2)
	{
	}

	void gameplay::on_character_contact_begin(world_handle character, world_handle other, const vector3& position, const vector3& normal)
	{
		check_managed_entities_collision(character, other);
	}

	void gameplay::on_character_contact(world_handle character, world_handle other, const vector3& position, const vector3& normal)
	{
	}

	void gameplay::on_character_contact_end(world_handle character, world_handle other)
	{
		(void)character;
		(void)other;
	}

	void gameplay::tick_player(float dt)
	{
		world&			   w	   = _app.get_world();
		component_manager& cm	   = w.get_comp_manager();
		auto&			   players = cm.underlying_pool<comp_cache<comp_player, MAX_PLAYERS>, comp_player>();
		for (comp_player& p : players)
			p.tick(w, dt);
	}

	void gameplay::tick_player_debug(float dt)
	{
		world&			   w	   = _app.get_world();
		component_manager& cm	   = w.get_comp_manager();
		auto&			   players = cm.underlying_pool<comp_cache<comp_player, MAX_PLAYERS>, comp_player>();
		for (comp_player& p : players)
			p.tick_debug(w, dt);
	}

	void gameplay::begin_player()
	{
		world&			   w	   = _app.get_world();
		component_manager& cm	   = w.get_comp_manager();
		auto&			   players = cm.underlying_pool<comp_cache<comp_player, MAX_PLAYERS>, comp_player>();
		for (comp_player& p : players)
		{
			p.begin_game(w, _app.get_main_window());
			_player_entity = p.get_header().entity;
		}
	}
}
