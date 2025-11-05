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
		engine_shader_type_hbao,
		engine_shader_type_hbao_upsample,
		engine_shader_type_bloom_downsample,
		engine_shader_type_bloom_upsample,
		engine_shader_type_post_combiner,
		engine_shader_type_object_id_write,
		engine_shader_type_max,
	};

	class game_app;

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

		bool init(gfx_id bind_layout, gfx_id bind_layout_compute, game_app* app);
		void uninit();

#ifdef SFG_TOOLMODE
		void tick();
		void add_reload_listener(reload_callback&& cb);
#endif

		inline const shader_direct& get_shader(uint8 type) const
		{
			return _shaders[type].direct;
		}

	private:
		struct shader_entry
		{
			shader_direct direct = {};
			gfx_id		  layout = 0;
#ifdef SFG_TOOLMODE
			string src_path = "";
#endif
		};

	private:
#ifdef SFG_TOOLMODE
		void on_shader_reloaded(const char* p, uint64 last_modified, uint16 id);
#endif

	private:
		static_vector<shader_entry, engine_shader_type_max> _shaders;
		simple_file_watcher									_file_watcher;
		game_app*											_app = nullptr;

#ifdef SFG_TOOLMODE
		static_vector<reload_callback, engine_shader_type_max> _reload_callbacks;
#endif
	};
}
