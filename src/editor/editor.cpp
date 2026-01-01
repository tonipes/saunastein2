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

// gfx
#include "gfx/renderer.hpp"

// platform
#include "platform/window.hpp"
#include "platform/process.hpp"

// world
#include "world/world.hpp"
#include "resources/world_raw.hpp"

// misc
#include "serialization/serialization.hpp"
#include "io/file_system.hpp"
#include "gui/vekt.hpp"
#include "input/input_mappings.hpp"
#include "project/engine_data.hpp"

namespace SFG
{

#define EDITOR_SETTINGS_FILE "/stakeforge/editor.stksettings"
	editor::editor(app& game) : _app(game)
	{
	}

	editor::~editor() = default;

	editor* editor::s_instance = nullptr;

	void editor::init()
	{
		s_instance = this;

		// -----------------------------------------------------------------------------
		// vekt init
		// -----------------------------------------------------------------------------

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
		_font_main	   = _font_manager->load_font_from_file(p.c_str(), 18);
#else
		SFG_NOTIMPLEMENTED();
#endif

		// -----------------------------------------------------------------------------
		// editor pipeline
		// -----------------------------------------------------------------------------
		const string editor_folder = file_system::get_user_directory() + "/stakeforge/";
		if (!file_system::exists(editor_folder.c_str()))
			file_system::create_directory(editor_folder.c_str());

		_theme.init(editor_folder.c_str());
		_layout.init(editor_folder.c_str());
		_settings.init(editor_folder.c_str());

		// -----------------------------------------------------------------------------
		// gui
		// -----------------------------------------------------------------------------

		_gui_world_overlays.init(_builder);
		_panel_controls.init(_builder);
		_panel_entities.init(_builder);
		_panel_properties.init(_builder);

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

		_renderer.draw_begin();
		_panel_entities.draw(w, ws);
		_panel_properties.draw(w, _selected_entity, ws);
		_panel_controls.draw({});
		_renderer.draw_end();

		// _builder->build_begin(vector2(ws.x, ws.y));
		// _panel_controls.draw({});
		// _builder->build_end();
		//
		// // notify buffer swap
		// _renderer.draw_end(_builder);
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

	void editor::load_level_prompt()
	{
		const string  file	   = process::select_file("load level", ".stkworld");
		const string& work_dir = engine_data::get().get_working_dir();
		const string  relative = file.substr(work_dir.size(), file.length() - work_dir.size());

		world_raw raw = {};
		raw.load_from_file(relative.c_str(), work_dir.c_str());

		world& w = _app.get_world();
		w.create_from_loader(raw);
	}

	void editor::load_level(const char* lvl)
	{
	}

	void editor::save_lavel()
	{
	}

}
