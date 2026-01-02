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

#include "editor_panel_properties.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "math/vector2ui16.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "gui/vekt.hpp"

namespace SFG
{
	static uint32 get_direct_child_count(entity_manager& em, world_handle e)
	{
		const entity_family& fam = em.get_entity_family(e);
		uint32				 c	 = 0;
		world_handle		 ch	 = fam.first_child;
		while (!ch.is_null())
		{
			c++;
			ch = em.get_entity_family(ch).next_sibling;
		}
		return c;
	}

	void editor_panel_properties::init()
	{
	}

	void editor_panel_properties::uninit()
	{
	}

	void editor_panel_properties::draw(world& w, world_handle selected, const vector2ui16& window_size)
	{
	}
}
