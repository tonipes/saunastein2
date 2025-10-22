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

		static_vector<const char*, engine_shader_type::engine_shader_type_max> paths;

		// ordered!
		paths.push_back("assets/engine/shaders/debug_controller/console_draw.stkshader");
		paths.push_back("assets/engine/shaders/gui/gui_default.stkshader");
		paths.push_back("assets/engine/shaders/gui/gui_sdf.stkshader");
		paths.push_back("assets/engine/shaders/gui/gui_text.stkshader");
		paths.push_back("assets/engine/shaders/swapchain/swapchain.stkshader");
		paths.push_back("assets/engine/shaders/lighting/deferred_lighting.stkshader");

		shader_raw raw = {};

		for (uint8 i = 0; i < engine_shader_type_max; i++)
		{
			const string  p = root + paths[i];
			shader_entry& e = _shaders[i];
			raw				= {};

#ifdef SFG_TOOLMODE
			if (!raw.load_from_file(p.c_str(), SFG_ROOT_DIRECTORY))
			{
				raw.destroy();
				return false;
			}

			e.src_path = p;
#else
			SFG_NOTIMPLEMENTED();
#endif

			const string src = root + raw.source.c_str();
			e.direct.create_from_loader(raw, bind_layout);

			_file_watcher.add_path(p.c_str(), static_cast<uint16>(i));
			_file_watcher.add_path(src.c_str(), static_cast<uint16>(i));
			raw.destroy();
		}

		_file_watcher.set_callback(std::bind(&engine_shaders::on_shader_reloaded, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		_file_watcher.set_tick_interval(60);
		return true;
	}

	void engine_shaders::uninit()
	{
		for (shader_entry& e : _shaders)
			e.direct.destroy();
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
		const engine_shader_type type  = static_cast<engine_shader_type>(id);
		shader_entry&			 entry = _shaders[id];
		shader_raw				 raw   = {};
		if (!raw.load_from_file(entry.src_path.c_str(), SFG_ROOT_DIRECTORY))
		{
			return;
		}

		game_app::get()->join_render();
		entry.direct.destroy();
		entry.direct.create_from_loader(raw, _bind_layout);
		raw.destroy();
		for (auto cb : _reload_callbacks)
			cb(type, entry.direct);
		game_app::get()->kick_off_render();
	}

#endif
}
