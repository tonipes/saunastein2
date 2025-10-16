// Copyright (c) 2025 Inan Evin

#include "world.hpp"
#include "io/log.hpp"
#include "io/file_system.hpp"
#include "project/engine_data.hpp"
#include "app/debug_console.hpp"

#ifdef SFG_TOOLMODE
#include <fstream>
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

/* DEBUG */
#include <algorithm>
#include <execution>
#include "gfx/renderer.hpp"
#include "resources/texture.hpp"
#include "resources/texture_raw.hpp"
#include "resources/shader.hpp"
#include "resources/material.hpp"
#include "resources/model.hpp"
#include "resources/skin.hpp"
#include "resources/mesh.hpp"
#include "resources/animation.hpp"
#include "platform/time.hpp"
#include "gfx/world/world_renderer.hpp"
#include "resources/model_raw.hpp"
#include "resources/model_node.hpp"
#include "resources/primitive.hpp"
#include "traits/trait_mesh_instance.hpp"
#include "math/math.hpp"

namespace SFG
{

	world::world(render_event_stream& rstream) : _entity_manager(*this), _resources(*this), _render_stream(rstream)
	{
		_text_allocator.init(MAX_ENTITIES * 32);
	};

	world::~world()
	{
		_text_allocator.uninit();
	}

	void world::init()
	{
		SFG_PROG("initializing world.");

		debug_console::get()->register_console_function<int>("world_set_play", [this](int b) { _flags.set(world_flags_is_playing, b != 0); });
		_flags.set(world_flags_is_init);
		_resources.init();
		_entity_manager.init();

		debug_console::get()->register_console_function<>("world_reload", [this]() {
			world_raw	 raw = {};
			const string p	 = engine_data::get().get_working_dir() + "assets/world/demo_world.stkworld";
			raw.cook_from_file(p.c_str());
			create_from_raw(raw);
		});
	}

	void world::uninit()
	{
		_entity_manager.uninit();
		_resources.uninit();
		_text_allocator.reset();

		SFG_PROG("uninitializing world.");

		debug_console::get()->unregister_console_function("world_reload");
		debug_console::get()->unregister_console_function("world_set_play");
		_flags.remove(world_flags_is_init);
	}

	void world::load_debug()
	{
		const int64 mr_begin = time::get_cpu_microseconds();

		world_raw	 raw = {};
		const string p	 = engine_data::get().get_working_dir() + "assets/world/demo_world.stkworld";
		raw.cook_from_file(p.c_str());
		create_from_raw(raw);

		const int64 mr_diff = time::get_cpu_microseconds() - mr_begin;
		SFG_INFO("Resources took: {0} ms", mr_diff / 1000);
	}

	void world::create_from_raw(world_raw& raw)
	{
		uninit();
		init();
		_resources.load_resources(raw.resources);
	}

	void world::tick(const vector2ui16& res, float dt)
	{
		_resources.tick();
	}

	void world::post_tick(double interpolation)
	{
		_entity_manager.post_tick(interpolation);
	}

	void world::pre_render(const vector2ui16& res)
	{
	}

#ifdef SFG_TOOLMODE

	void world::save(const char* path)
	{
		json j;
		j["dummy"] = 2;

		std::ofstream file(path);

		if (file.is_open())
		{
			file << j.dump(4);
			file.close();
		}

		SFG_PROG("saved world: {0}", path);
	}

	void world::load(const char* path)
	{
		if (strlen(path) == 0)
			return;

		if (_flags.is_set(world_flags_is_init))
			uninit();

		if (!file_system::exists(path))
		{
			SFG_ERR("can't load world as path don't exist! {0}", path);
			return;
		}

		std::ifstream f(path);
		json		  data	= json::parse(f);
		int			  dummy = data["dummy"];
		SFG_PROG("loaded world {0}", path);

		init();
	}
#endif

	bool world::on_window_event(const window_event& ev)
	{
		if (!_flags.is_set(world_flags_is_init))
			return false;
		return false;
	}
}
