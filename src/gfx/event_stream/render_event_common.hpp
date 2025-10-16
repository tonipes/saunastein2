// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"

namespace SFG
{

	enum render_event_type : uint8
	{
		render_event_create_texture = 0,
		render_event_create_sampler,
		render_event_create_material,
		render_event_create_mesh,
		render_event_create_shader,
		render_event_destroy_texture,
		render_event_destroy_sampler,
		render_event_destroy_material,
		render_event_destroy_mesh,
		render_event_destroy_shader,
		render_event_create_model,
		render_event_destroy_model,
		render_event_update_material,
		render_event_update_mesh_instance,
		render_event_remove_mesh_instance,
		render_event_update_entity_transform,
		render_event_update_entity_visibility,
		render_event_set_main_camera,
		render_event_update_camera,
		render_event_remove_camera,
		render_event_reload_shader,
		render_event_reload_texture,
		render_event_reload_material,
	};

}
