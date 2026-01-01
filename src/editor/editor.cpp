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

#include "editor.hpp"

// app
#include "app/app.hpp"
#include "app/debug_console.hpp"

#include "platform/window.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "common/string_id.hpp"
#include "project/engine_data.hpp"
#include "input/input_mappings.hpp"

// game
#include "game/game_world_renderer.hpp"

// gfx
#include "gfx/renderer.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "gfx/event_stream/render_event_stream.hpp"

// world
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "world/components/comp_camera.hpp"
#include "world/components/comp_model_instance.hpp"
#include "world/components/comp_ambient.hpp"
#include "world/components/comp_light.hpp"
#include "world/components/comp_physics.hpp"
#include "world/components/comp_audio.hpp"
#include "world/components/comp_canvas.hpp"

// resources
#include "resources/model.hpp"
#include "resources/model_raw.hpp"
#include "resources/world_raw.hpp"
#include "resources/material.hpp"
#include "resources/audio.hpp"
#include "resources/font.hpp"

namespace SFG
{
	editor::editor(app& game) : _app(game)
	{
	}

	editor::~editor() = default;

	void editor::init()
	{

		// world_raw raw = {};
		// raw.load_from_file("assets/world/demo_world.stkworld", engine_data::get().get_working_dir().c_str());
		// _game.get_world().create_from_loader(raw);

		/*
		{
			world&			  w	 = _game.get_world();
			entity_manager&	  em = w.get_entity_manager();
			component_manager&	  tm = w.get_comp_manager();
			resource_manager& rm = w.get_resource_manager();

			rm.load_resources(
				{
					"assets/engine/shaders/gizmo/gizmo.stkshader",
					"assets/engine/materials/gizmo_x.stkmat",
					"assets/engine/materials/gizmo_y.stkmat",
					"assets/engine/materials/gizmo_z.stkmat",
					"assets/engine/models/gizmos/gizmos.stkmodel",
				},
				false,
				SFG_ROOT_DIRECTORY);

			// gizmo model (all).
			resource_handle gizmos_handle = rm.get_resource_handle_by_hash<model>(TO_SIDC("assets/engine/models/gizmos/gizmos.stkmodel"));

			// get gizmo materials.
			const resource_handle gizmo_mat0 = rm.get_resource_handle_by_hash<material>(TO_SIDC("assets/engine/materials/gizmo_x.stkmat"));
			const resource_handle gizmo_mat1 = rm.get_resource_handle_by_hash<material>(TO_SIDC("assets/engine/materials/gizmo_y.stkmat"));
			const resource_handle gizmo_mat2 = rm.get_resource_handle_by_hash<material>(TO_SIDC("assets/engine/materials/gizmo_z.stkmat"));

			// update model materials.
			const render_event_model_update_materials ev = {
				.materials =
					{
						gizmo_mat1.index,
						gizmo_mat2.index,
						gizmo_mat0.index,
					},
			};
			w.get_render_stream().add_event({.index = gizmos_handle.index, .event_type = update_model_materials}, ev);

			_gizmo_entity						   = em.create_entity("gizmos");
			world_handle		  gizmos_mi_handle = tm.add_component<comp_model_instance>(_gizmo_entity);
			comp_model_instance& gizmos_mi		   = tm.get_component<comp_model_instance>(gizmos_mi_handle);
			gizmos_mi.instantiate_model_to_world(w, gizmos_handle);
		}*/

#ifdef SFG_TOOLMODE
		debug_console::get()->register_console_function("start_playmode", [this]() { _app.get_world().set_playmode(play_mode::full); });
		debug_console::get()->register_console_function("stop_playmode", [this]() { _app.get_world().set_playmode(play_mode::none); });
		debug_console::get()->register_console_function("start_physics", [this]() { _app.get_world().set_playmode(play_mode::physics_only); });
		debug_console::get()->register_console_function("stop_physics", [this]() { _app.get_world().set_playmode(play_mode::none); });
		debug_console::get()->register_console_function<const char*>("load_level", [this](const char* lvl) {

		});
#endif

		_builder = new vekt::builder();
		_builder->init({
			.vertex_buffer_sz			 = 1024 * 1024 * 4,
			.index_buffer_sz			 = 1024 * 1024 * 8,
			.text_cache_vertex_buffer_sz = 1024 * 1024 * 2,
			.text_cache_index_buffer_sz	 = 1024 * 1024 * 4,
			.buffer_count				 = 12,
		});

		_font_manager = new vekt::font_manager();
		_font_manager->init();
		_font_manager->set_callback_user_data(&_renderer);
		_font_manager->set_atlas_created_callback(editor_renderer::on_atlas_created);
		_font_manager->set_atlas_updated_callback(editor_renderer::on_atlas_updated);
		_font_manager->set_atlas_destroyed_callback(editor_renderer::on_atlas_destroyed);

		_renderer.init(_app.get_main_window(), _app.get_renderer().get_texture_queue());

#ifdef SFG_TOOLMODE
		const string p = SFG_ROOT_DIRECTORY + string("assets/engine/fonts/VT323-Regular.ttf");
		_font_main = _font_manager->load_font_from_file(p.c_str(), 18);
#else
		SFG_NOTIMPLEMENTED();
#endif

		// gui
		_gui_world_overlays.init(_builder);
		_panel_controls.init(_builder);

		_camera_controller.init(_app.get_world(), _app.get_main_window());
	}

	void editor::uninit()
	{
		_font_manager->unload_font(_font_main);
		_font_manager->uninit();
		delete _font_manager;
		_font_manager = nullptr;

		_builder->uninit();
		delete _builder;
		_builder = nullptr;

		_renderer.uninit();

		_camera_controller.uninit();
	}

	void editor::pre_world_tick(float delta)
	{
	}

	void editor::post_world_tick(float delta)
	{
		_camera_controller.tick(delta);
	}

	void editor::tick()
	{
		world&			   w  = _app.get_world();
		const vector2ui16& ws = _app.get_main_window().get_size();

		_builder->build_begin(vector2(ws.x, ws.y));
		_panel_controls.draw({});
		_builder->build_end();

		// notify buffer swap
		_renderer.draw_end(_builder);
	}

	void editor::render(const render_params& p)
	{
		_renderer.prepare(p.pm, p.cmd_buffer, p.frame_index);
		_renderer.render({
			.cmd_buffer	   = p.cmd_buffer,
			.frame_index   = p.frame_index,
			.alloc		   = p.alloc,
			.size		   = p.size,
			.global_layout = p.global_layout,
			.global_group  = p.global_group,
		});
	}

	bool editor::on_window_event(const window_event& ev)
	{
		if (_camera_controller.on_window_event(ev))
			return true;

		if (ev.type == window_event_type::mouse && ev.sub_type == window_event_sub_type::press)
		{
			if (ev.button == input_code::mouse_0 && ev.value.x > 0 && ev.value.y > 0)
			{
				// const uint32	   id	= _game.get_renderer()->get_world_renderer()->get_render_pass_object_id().read_location(ev.value.x, ev.value.y, 0);
				// const entity_meta& meta = _game.get_world().get_entity_manager().get_entity_meta({.generation = 2, .index = id});
				// _game.get_world().get_entity_manager().set_entity_position(_gizmo_entity, _game.get_world().get_entity_manager().get_entity_position_abs({.generation = 2, .index = id}));
				// _game.get_world().get_entity_manager().teleport_entity(_gizmo_entity);
				// _game.get_renderer()->get_world_renderer()->get_render_pass_selection_outline().set_selected_entity_id(id == 0 ? NULL_WORLD_ID : id);
				// SFG_WARN("Pressed on object {0} - name: {1}", id, meta.name);
			}
		}

		return false;
	}

	void editor::resize(const vector2ui16& size)
	{
		_renderer.resize(size);
	}

}
