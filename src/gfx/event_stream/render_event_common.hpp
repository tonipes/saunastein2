// Copyright (c) 2025 Inan Evin

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
		destroy_texture,
		destroy_sampler,
		destroy_material,
		destroy_mesh,
		destroy_shader,
		create_model,
		update_model_materials,
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
		reload_texture,
		reload_material,
		reload_sampler,
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
