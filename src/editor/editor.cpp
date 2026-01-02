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
#include "editor_settings.hpp"
#include "editor_theme.hpp"
#include "editor_layout.hpp"

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

// resources
#include "resources/resource_manager.hpp"
#include "resources/model.hpp"

// components
#include "world/components/comp_model_instance.hpp"

// misc
#include "serialization/serialization.hpp"
#include "io/file_system.hpp"
#include "gui/vekt.hpp"
#include "input/input_mappings.hpp"
#include "math/math.hpp"

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

		constexpr size_t VTX_SZ = 1024 * 1024 * 4;
		constexpr size_t IDX_SZ = 1024 * 1024 * 4;
		_builder->init({
			.vertex_buffer_sz			 = VTX_SZ,
			.index_buffer_sz			 = IDX_SZ,
			.text_cache_vertex_buffer_sz = 1024 * 1024 * 2,
			.text_cache_index_buffer_sz	 = 1024 * 1024 * 4,
			.buffer_count				 = 12,
		});

		_builder->add_input_layer(0, _builder->get_root());

		_font_manager = new vekt::font_manager();
		_font_manager->init();
		_font_manager->set_callback_user_data(&_renderer);
		_font_manager->set_atlas_created_callback(editor_renderer::on_atlas_created);
		_font_manager->set_atlas_updated_callback(editor_renderer::on_atlas_updated);
		_font_manager->set_atlas_destroyed_callback(editor_renderer::on_atlas_destroyed);

		_renderer.init(_app.get_main_window(), _app.get_renderer().get_texture_queue(), VTX_SZ, IDX_SZ);

		const string default_font_str = SFG_ROOT_DIRECTORY + string("assets/engine/fonts/VT323-Regular.ttf");
		const string title_font_str	  = SFG_ROOT_DIRECTORY + string("assets/engine/fonts/VT323-Regular.ttf");

		_font_main	= _font_manager->load_font_from_file(default_font_str.c_str(), 28);
		_font_title = _font_manager->load_font_from_file(title_font_str.c_str(), 36);

		_camera_controller.init(_app.get_world(), _app.get_main_window());

		// -----------------------------------------------------------------------------
		// editor pipeline
		// -----------------------------------------------------------------------------

		if (!file_system::exists(editor_settings::get().working_dir.c_str()))
		{
			editor_settings::get().working_dir = process::select_folder("select project directory");
			file_system::fix_path(editor_settings::get().working_dir);
			editor_settings::get().save_last();

			if (!file_system::exists(editor_settings::get().working_dir.c_str()))
			{
				SFG_ASSERT(false);
			}
		}
		editor_theme::get().init(editor_settings::get()._editor_folder.c_str());
		editor_layout::get().init(editor_settings::get()._editor_folder.c_str());

		editor_theme::get().font_default = _font_main;
		editor_theme::get().font_title	 = _font_title;

		// _app.get_world().uninit();
		// _app.get_world().init();

		_app.get_world().init();
		_camera_controller.activate();

		// if (!editor_settings::get().last_world_relative.empty())
		//{
		//	const string last_world = editor_settings::get().working_dir + editor_settings::get().last_world_relative;
		//
		//	if (file_system::exists(last_world.c_str()))
		//		load_level(editor_settings::get().last_world_relative.c_str());
		//	else
		//	{
		//		_app.get_world().init();
		//		_camera_controller.activate();
		//	}
		// }
		// else
		//{
		//	_app.get_world().init();
		//	_camera_controller.activate();
		// }

		// -----------------------------------------------------------------------------
		// gui
		// -----------------------------------------------------------------------------

		_gui_world_overlays.init(_builder);
		_panel_controls.init(_builder);
		_panel_entities.init();
		_panel_properties.init();
		_panel_world_view.init();
		_panels_docking.init();
	}

	void editor::uninit()
	{
		editor_theme::get().font_default = nullptr;
		editor_theme::get().font_title	 = nullptr;

		_font_manager->unload_font(_font_main);
		_font_manager->unload_font(_font_title);
		_font_manager->uninit();
		delete _font_manager;
		_font_manager = nullptr;

		_builder->uninit();
		delete _builder;
		_builder = nullptr;

		_renderer.uninit();

		_camera_controller.uninit();
		_panel_world_view.uninit();
		_panels_docking.uninit();
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
		window&				  wnd	= _app.get_main_window();
		const vector<string>& drops = wnd.get_dropped_files();
		for (const string& str : drops)
		{
			on_file_dropped(str.c_str());
		}
		wnd.clear_dropped_files();

		world&			   w  = _app.get_world();
		const vector2ui16& ws = _app.get_main_window().get_size();
		vector2ui16		   world_res;
		if (_panel_world_view.consume_committed_size(world_res))
			_app.set_game_resolution(world_res);

		// _renderer.draw_begin();
		// _panels_docking.draw(ws);
		// _panel_entities.draw(w, ws);
		// _panel_properties.draw(w, _selected_entity, ws);
		// _panel_controls.draw(ws);
		// _panel_world_view.draw(ws);
		// _renderer.draw_end();

		_panel_controls.draw(ws, _builder);
		_builder->build_begin(vector2(ws.x, ws.y));
		_builder->build_end();

		// notify buffer swap
		_renderer.draw_end(_builder);
	}

	void editor::render(const render_params& p)
	{
		_world_rt_gpu_index.store(p.world_rt_index, std::memory_order_release);
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

		if (ev.type == window_event_type::delta)
		{
			const vector2i16& mp = _app.get_main_window().get_mouse_position();
			_builder->on_mouse_move(vector2(mp.x, mp.y));
		}
		if (ev.type == window_event_type::mouse && ev.sub_type == window_event_sub_type::press)
		{
			_builder->on_mouse_event({
				.type	  = static_cast<vekt::input_event_type>(ev.sub_type),
				.button	  = ev.button,
				.position = VEKT_VEC2(ev.value.x, ev.value.y),
			});

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

	void editor::on_file_dropped(const char* path)
	{
		const string& wd = editor_settings::get().working_dir;

		string file = path;
		file_system::fix_path(file);

		if (file.find(wd.c_str()) != 0)
		{
			SFG_ERR("file should be in project directory.");
			return;
		}

		const string relative = file.substr(wd.size(), file.length() - wd.size());
		const string ext	  = file_system::get_file_extension(relative);

		world&			   w  = _app.get_world();
		resource_manager&  rm = w.get_resource_manager();
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();

		if (ext.compare("stkworld") == 0)
		{
			load_level(relative.c_str());
			return;
		}

		if (ext.compare("stkmodel") == 0)
		{
			const string_id sid = TO_SID(relative);

			resource_handle handle = rm.get_resource_handle_by_hash_if_exists<model>(sid);

			if (handle.is_null())
			{
				rm.load_resources({relative}, false, wd.c_str());
				handle = rm.get_resource_handle_by_hash<model>(sid);
			}

			if (handle.is_null())
			{
				SFG_ERR("something went wrong loading resource: {0}", relative.c_str());
				return;
			}

			const string		 name	= file_system::get_filename_from_path(relative);
			const world_handle	 entity = em.create_entity(name.c_str());
			const world_handle	 inst	= cm.add_component<comp_model_instance>(entity);
			comp_model_instance& mi		= cm.get_component<comp_model_instance>(inst);
			mi.instantiate_model_to_world(w, handle);
			return;
		}

		SFG_ERR("dropped file with unknown extension: {0}", ext.c_str());
	}

	const vector2ui16& editor::get_game_resolution() const
	{
		return _app.get_game_resolution();
	}

	void editor::load_level_prompt()
	{
		string file = process::select_file("load level", ".stkworld");
		file_system::fix_path(file);

		const string& work_dir = editor_settings::get().working_dir;
		if (file.find(work_dir.c_str()) != 0)
		{
			SFG_ERR("level should be in project directory.");
			return;
		}
		const string relative					   = file.substr(work_dir.size(), file.length() - work_dir.size());
		editor_settings::get().last_world_relative = relative;
		editor_settings::get().save_last();
		load_level(relative.c_str());
	}

	void editor::load_level(const char* relative_path)
	{
		const string file = relative_path;

		world_raw raw = {};
		raw.load_from_file(relative_path, editor_settings::get().working_dir.c_str());

		_camera_controller.deactivate();

		world& w = _app.get_world();
		w.create_from_loader(raw);

		_camera_controller.activate();
	}

	void editor::save_lavel()
	{
	}

}
