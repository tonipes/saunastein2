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

#include "common/size_definitions.hpp"

namespace SFG
{
	enum class render_event_type : uint8
	{
		create_texture = 0,
		create_sampler,
		create_material,
		create_mesh,
		create_shader,
		create_skin,
		destroy_texture,
		destroy_sampler,
		destroy_material,
		destroy_mesh,
		destroy_shader,
		destroy_skin,
		create_model,
		update_model_materials,
		update_material_sampler,
		destroy_model,
		update_material,
		update_mesh_instance,
		remove_mesh_instance,
		update_entity_transform,
		update_entity_visibility,
		set_main_camera,
		update_camera,
		remove_camera,
		reload_shader,
		reload_material,
		update_ambient,
		update_dir_light,
		update_point_light,
		update_spot_light,
		remove_ambient,
		remove_dir_light,
		remove_point_light,
		remove_spot_light,
		create_canvas,
		destroy_canvas,
		canvas_add_draw,
		canvas_reset_draws,
		canvas_update,
	};

}
