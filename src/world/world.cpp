// Copyright (c) 2025 Inan Evin

#include "world.hpp"
#include "world/world_max_defines.hpp"

// resources
#include "resources/texture.hpp"
#include "resources/texture_raw.hpp"
#include "resources/texture_sampler.hpp"
#include "resources/texture_sampler_raw.hpp"
#include "resources/shader.hpp"
#include "resources/shader_raw.hpp"
#include "resources/material.hpp"
#include "resources/material_raw.hpp"
#include "resources/model.hpp"
#include "resources/model_raw.hpp"
#include "resources/skin.hpp"
#include "resources/skin_raw.hpp"
#include "resources/mesh.hpp"
#include "resources/mesh_raw.hpp"
#include "resources/animation.hpp"
#include "resources/animation_raw.hpp"
#include "resources/audio.hpp"
#include "resources/audio_raw.hpp"
#include "resources/font.hpp"
#include "resources/font_raw.hpp"
#include "resources/physical_material.hpp"
#include "resources/physical_material_raw.hpp"
#include "resources/model_node.hpp"
#include "resources/primitive.hpp"
#include "resources/world_raw.hpp"

// traits
#include "traits/trait_camera.hpp"
#include "traits/trait_light.hpp"
#include "traits/trait_mesh_instance.hpp"
#include "traits/trait_model_instance.hpp"
#include "traits/trait_ambient.hpp"
#include "traits/trait_physics.hpp"

#include "app/debug_console.hpp"

namespace SFG
{
	world::world(render_event_stream& rstream) : _entity_manager(*this), _trait_manager(*this), _render_stream(rstream), _resource_manager(*this), _phy_world(*this)
	{
		_text_allocator.init(MAX_ENTITIES * 32);

		// resource registry
		_resource_manager.register_cache<texture, texture_raw, MAX_WORLD_TEXTURES, 0>();
		_resource_manager.register_cache<texture_sampler, texture_sampler_raw, MAX_WORLD_SAMPLERS, 0>();
		_resource_manager.register_cache<audio, audio_raw, MAX_WORLD_AUDIO, 0>();
		_resource_manager.register_cache<font, font_raw, MAX_WORLD_FONTS, 0>();
		_resource_manager.register_cache<material, material_raw, MAX_WORLD_MATERIALS, 1>();
		_resource_manager.register_cache<mesh, mesh_raw, MAX_WORLD_MESHES, 0>();
		_resource_manager.register_cache<model, model_raw, MAX_WORLD_MODELS, 1>();
		_resource_manager.register_cache<animation, animation_raw, MAX_WORLD_ANIMS, 0>();
		_resource_manager.register_cache<skin, skin_raw, MAX_WORLD_SKINS, 0>();
		_resource_manager.register_cache<shader, shader_raw, MAX_WORLD_SHADERS, 0>();
		_resource_manager.register_cache<physical_material, physical_material_raw, MAX_WORLD_PHYSICAL_MATERIALS, 0>();

		// trait registry
		_trait_manager.register_cache<trait_camera, MAX_WORLD_TRAIT_CAMERAS>();
		_trait_manager.register_cache<trait_point_light, MAX_WORLD_TRAIT_POINT_LIGHTS>();
		_trait_manager.register_cache<trait_spot_light, MAX_WORLD_TRAIT_SPOT_LIGHTS>();
		_trait_manager.register_cache<trait_dir_light, MAX_WORLD_TRAIT_DIR_LIGHTS>();
		_trait_manager.register_cache<trait_model_instance, MAX_WORLD_TRAIT_MODEL_INSTANCES>();
		_trait_manager.register_cache<trait_mesh_instance, MAX_WORLD_TRAIT_MESH_INSTANCES>();
		_trait_manager.register_cache<trait_ambient, MAX_WORLD_TRAIT_AMBIENTS>();
		_trait_manager.register_cache<trait_physics, MAX_WORLD_TRAIT_PHYSICS>();

		_phy_world.init();

#ifdef SFG_TOOLMODE
		debug_console::get()->register_console_function("start_playmode", [this]() { start_playmode(); });
		debug_console::get()->register_console_function("stop_playmode", [this]() { stop_playmode(); });
		debug_console::get()->register_console_function("start_physics", [this]() { start_physics(); });
		debug_console::get()->register_console_function("stop_physics", [this]() { stop_physics(); });
#endif
	};

	world::~world()
	{
		_text_allocator.uninit();
		_phy_world.uninit();
	}

	void world::init()
	{
		_flags.set(world_flags_is_init);
		_resource_manager.init();
		_entity_manager.init();
		_trait_manager.init();
	}

	void world::uninit()
	{
		_trait_manager.uninit();
		_entity_manager.uninit();
		_resource_manager.uninit();
		_text_allocator.reset();
		_flags.remove(world_flags_is_init);
	}

	void world::create_from_loader(world_raw& raw)
	{
		uninit();
		init();
		_resource_manager.load_resources(raw.resources);
	}

	void world::tick(const vector2ui16& res, float dt)
	{
		_resource_manager.tick();

		if (_flags.is_set(world_flags_is_physics_active))
			_phy_world.simulate(dt);
	}

	void world::post_tick(double interpolation)
	{
		_entity_manager.post_tick(interpolation);
	}

	bool world::on_window_event(const window_event& ev)
	{
		if (!_flags.is_set(world_flags_is_init))
			return false;
		return false;
	}

	void world::start_playmode()
	{
		if (_flags.is_set(world_flags_is_playing))
		{
			SFG_ERR("Can't start playmode as already playing.");
			return;
		}

		if (_flags.is_set(world_flags_is_physics_active))
		{
			SFG_ERR("Can't start playmode as simulating physics.");
			return;
		}

		_phy_world.init_simulation();

		_flags.set(world_flags_is_playing | world_flags_is_physics_active);
	}

	void world::stop_playmode()
	{
		if (!_flags.is_set(world_flags_is_playing))
		{
			SFG_ERR("Can't end playmode as its not active.");
			return;
		}

		_phy_world.uninit_simulation();

		_flags.remove(world_flags_is_playing | world_flags_is_physics_active);
	}

	void world::start_physics()
	{
		if (_flags.is_set(world_flags_is_playing))
		{
			SFG_ERR("Can't start physics as already playing.");
			return;
		}

		if (_flags.is_set(world_flags_is_physics_active))
		{
			SFG_ERR("Can't start physics as already simulating physics.");
			return;
		}

		_flags.set(world_flags_is_physics_active);

		_phy_world.init_simulation();
	}

	void world::stop_physics()
	{
		if (!_flags.is_set(world_flags_is_physics_active))
		{
			SFG_ERR("Can't end physics as its not active.");
			return;
		}
		_flags.remove(world_flags_is_physics_active);

		_phy_world.uninit_simulation();
	}
}
