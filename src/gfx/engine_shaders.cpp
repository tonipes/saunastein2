// Copyright (c) 2025 Inan Evin

#include "engine_shaders.hpp"
#include "resources/shader_raw.hpp"
#include "data/string.hpp"
#include "app/game_app.hpp"

namespace SFG
{

	bool engine_shaders::init(gfx_id bind_layout)
	{
		_bind_layout = bind_layout;

		const string root = SFG_ROOT_DIRECTORY;

		_shaders.resize(engine_shader_type::engine_shader_type_max);

		static_vector<string, engine_shader_type::engine_shader_type_max> paths;

		// ordered!
		paths.push_back(root + "assets/engine/shaders/debug_controller/console_draw.stkshader");
		paths.push_back(root + "assets/engine/shaders/gui/gui_default.stkshader");
		paths.push_back(root + "assets/engine/shaders/gui/gui_sdf.stkshader");
		paths.push_back(root + "assets/engine/shaders/gui/gui_text.stkshader");
		paths.push_back(root + "assets/engine/shaders/swapchain/swapchain.stkshader");
		paths.push_back(root + "assets/engine/shaders/lighting/deferred_lighting.stkshader");

		shader_raw raw = {};

		for (uint8 i = 0; i < engine_shader_type_max; i++)
		{
			const string p = paths[i];
			raw			   = {};

#ifdef SFG_TOOLMODE
			if (!raw.cook_from_file(p.c_str(), false, bind_layout, true))
			{
				raw.destroy();
				return false;
			}
#else
			SFG_NOTIMPLEMENTED();
#endif

			_shaders[engine_shader_type_debug_console].create_from_raw(raw);
			_file_watcher.add_path(p.c_str(), static_cast<uint16>(i));
			raw.destroy();
		}

		_file_watcher.set_callback(std::bind(&engine_shaders::on_shader_reloaded, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		_file_watcher.set_tick_interval(60);
		return true;
	}

	void engine_shaders::uninit()
	{
		for (shader_direct& sh : _shaders)
			sh.destroy();
	}
#ifdef SFG_TOOLMODE

	void engine_shaders::add_reload_listener(reload_callback&& cb)
	{
		_reload_callbacks.push_back(std::move(cb));
	}

	void engine_shaders::tick()
	{
		_file_watcher.tick();
	}

	void engine_shaders::on_shader_reloaded(const char* p, uint64 last_modified, uint16 id)
	{
		game_app::get()->join_render();
		const engine_shader_type type = static_cast<engine_shader_type>(id);
		shader_direct&			 sha  = _shaders[engine_shader_type_debug_console];
		sha.destroy();
		shader_raw raw = {};
		raw.cook_from_file(p, false, _bind_layout, false);
		sha.create_from_raw(raw);

		for (auto cb : _reload_callbacks)
			cb(type, sha);

		game_app::get()->kick_off_render();
	}

#endif
}
