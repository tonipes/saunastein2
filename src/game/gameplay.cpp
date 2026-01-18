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
#include "world/world.hpp"
#include "world/components/comp_camera.hpp"
namespace SFG
{
	void gameplay::on_world_begin(world& w)
	{
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();

		const world_handle cam_entity = em.create_entity("camera");
		const world_handle cam_comp	  = cm.add_component<comp_camera>(cam_entity);
		comp_camera&	   comp		  = cm.get_component<comp_camera>(cam_comp);
		comp.set_main(w);
		comp.set_values(w, 0.1, 500.0f, 90.0f);

		em.set_entity_position(cam_entity, vector3(0, 2, 15));
	}

	void gameplay::on_world_end(world& w)
	{
	}

	void gameplay::on_world_tick(world& w, float dt)
	{
	}
}