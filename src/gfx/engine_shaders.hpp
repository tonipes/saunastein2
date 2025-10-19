// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/static_vector.hpp"
#include "resources/shader_direct.hpp"
#include "io/simple_file_watcher.hpp"
#include "data/static_vector.hpp"
#include <functional>

namespace SFG
{

	enum engine_shader_type : uint8
	{
		engine_shader_type_debug_console = 0,
		engine_shader_type_gui_default,
		engine_shader_type_gui_sdf,
		engine_shader_type_gui_text,
		engine_shader_type_renderer_swapchain,
		engine_shader_type_world_lighting,
		engine_shader_type_max,
	};

	class engine_shaders
	{
	public:
#ifdef SFG_TOOLMODE
		typedef std::function<void(engine_shader_type type, shader_direct& sh)> reload_callback;
#endif

		static engine_shaders& get()
		{
			static engine_shaders inst;
			return inst;
		}

		bool init(gfx_id bind_layout);
		void uninit();

#ifdef SFG_TOOLMODE
		void tick();
		void add_reload_listener(reload_callback&& cb);
#endif

		inline const shader_direct& get_shader(uint8 type) const
		{
			return _shaders[type];
		}

	private:
#ifdef SFG_TOOLMODE
		void on_shader_reloaded(const char* p, uint64 last_modified, uint16 id);
#endif

	private:
		static_vector<shader_direct, engine_shader_type_max> _shaders;
		simple_file_watcher									 _file_watcher;
		gfx_id												 _bind_layout = 0;

#ifdef SFG_TOOLMODE
		static_vector<reload_callback, engine_shader_type_max> _reload_callbacks;
#endif
	};
}
