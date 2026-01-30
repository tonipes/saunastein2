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

#include "comp_player.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"
#include "reflection/reflection.hpp"

namespace SFG
{
	void comp_player::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_player>::value, 0, "component");
		m.set_title("player");
		m.set_category("game");
		m.add_field<&comp_player::_movement_speed, comp_player>("movement_speed", reflected_field_type::rf_float, "");
		m.add_field<&comp_player::_rotation_speed, comp_player>("rotation_speed", reflected_field_type::rf_float, "");

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) { comp_player* c = static_cast<comp_player*>(params.object_ptr); });

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) { comp_player* c = static_cast<comp_player*>(obj); });
	}

	void comp_player::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);
	}

	void comp_player::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);
	}

	void comp_player::set_values(world& w, const color& base_color)
	{
	}

}
