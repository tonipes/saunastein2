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
#include "data/static_vector.hpp"
#include "resources/shader_direct.hpp"
#include "io/simple_file_watcher.hpp"
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
		engine_shader_type_object_outline_write,
		engine_shader_debug_default,
		engine_shader_debug_line,
		engine_shader_particle_clear,
		engine_shader_particle_sim,
		engine_shader_particle_emit,
		engine_shader_particle_write_count,
		engine_shader_particle_count,
		engine_shader_particle_swap,
		engine_shader_type_max,
	};

	class app;

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

		bool init(gfx_id bind_layout, gfx_id bind_layout_compute, app* app);
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
		static void on_shader_reloaded(const char* p, uint64 last_modified, uint16 id, void* user_data);
#endif

	private:
		static_vector<shader_entry, engine_shader_type_max> _shaders;
		simple_file_watcher									_file_watcher;
		app*												_app = nullptr;

#ifdef SFG_TOOLMODE
		static_vector<reload_callback, engine_shader_type_max> _reload_callbacks;
#endif
	};
}
