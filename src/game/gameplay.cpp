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

namespace SFG
{

#define MAX_PLAYERS 1

	void gameplay::init()
	{
		_app.get_world().get_comp_manager().register_cache<comp_player, MAX_PLAYERS>();
	}

	void gameplay::uninit()
	{
	}

	void gameplay::on_world_begin(world& w)
	{
		begin_player();
	}

	void gameplay::on_world_end(world& w)
	{
	}

	void gameplay::on_world_tick(world& w, float dt, const vector2ui16& game_res)
	{
		tick_player(dt);
	}

	void gameplay::on_window_event(const window_event& ev, window* wnd)
	{
		world&			   w	   = _app.get_world();
		component_manager& cm	   = w.get_comp_manager();
		auto&			   players = cm.underlying_pool<comp_cache<comp_player, MAX_PLAYERS>, comp_player>();
		for (comp_player& p : players)
			p.on_window_event(ev);
	}

	void gameplay::tick_player(float dt)
	{
		world&			   w	   = _app.get_world();
		component_manager& cm	   = w.get_comp_manager();
		auto&			   players = cm.underlying_pool<comp_cache<comp_player, MAX_PLAYERS>, comp_player>();
		for (comp_player& p : players)
			p.tick(w, dt);
	}

	void gameplay::begin_player()
	{
		world&			   w	   = _app.get_world();
		component_manager& cm	   = w.get_comp_manager();
		auto&			   players = cm.underlying_pool<comp_cache<comp_player, MAX_PLAYERS>, comp_player>();
		for (comp_player& p : players)
			p.begin_game(w);
	}
}
