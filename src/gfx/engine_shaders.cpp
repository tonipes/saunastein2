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

#include "engine_shaders.hpp"
#include "resources/shader_raw.hpp"
#include "data/string.hpp"
#include "app/app.hpp"

namespace SFG
{

	bool engine_shaders::init(gfx_id bind_layout, gfx_id bind_layout_compute, app* app)
	{
		_app = app;

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
		paths.push_back("assets/engine/shaders/ssao/hbao.stkshader");
		paths.push_back("assets/engine/shaders/ssao/hbao_upsample.stkshader");
		paths.push_back("assets/engine/shaders/bloom/bloom_downsample.stkshader");
		paths.push_back("assets/engine/shaders/bloom/bloom_upsample.stkshader");
		paths.push_back("assets/engine/shaders/post_combiner/post_combiner.stkshader");
		paths.push_back("assets/engine/shaders/selection/selection_outline_write.stkshader");
		paths.push_back("assets/engine/shaders/debug/debug_default.stkshader");
		paths.push_back("assets/engine/shaders/debug/debug_line.stkshader");

		shader_raw raw = {};

		for (uint8 i = 0; i < engine_shader_type_max; i++)
		{
			const string  p = paths[i];
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

			const string src_stk_shader = root + p;
			const string src_source		= root + raw.source;

			const bool is_compute = (i == engine_shader_type_hbao || i == engine_shader_type_hbao_upsample || i == engine_shader_type_bloom_downsample || i == engine_shader_type_bloom_upsample);

			e.layout = is_compute ? bind_layout_compute : bind_layout;
			e.direct.create_from_loader(raw, e.layout);
			_file_watcher.add_path(src_source.c_str(), static_cast<uint16>(i));
			_file_watcher.add_path(src_stk_shader.c_str(), static_cast<uint16>(i));
			raw.destroy();
		}

		_file_watcher.set_callback(engine_shaders::on_shader_reloaded, this);
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

	void engine_shaders::on_shader_reloaded(const char* p, uint64 last_modified, uint16 id, void* user_data)
	{
		engine_shaders*			 es	   = static_cast<engine_shaders*>(user_data);
		const engine_shader_type type  = static_cast<engine_shader_type>(id);
		shader_entry&			 entry = es->_shaders[id];
		shader_raw				 raw   = {};
		if (!raw.load_from_file(entry.src_path.c_str(), SFG_ROOT_DIRECTORY))
		{
			return;
		}

		es->_app->join_render();
		entry.direct.destroy();
		entry.direct.create_from_loader(raw, entry.layout);
		raw.destroy();
		for (auto cb : es->_reload_callbacks)
			cb(type, entry.direct);
		es->_app->kick_off_render();
	}

#endif
}
