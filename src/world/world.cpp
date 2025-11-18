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
#include "traits/trait_audio.hpp"
#include "traits/trait_canvas.hpp"

#include "app/debug_console.hpp"

namespace SFG
{
	world::world(render_event_stream& rstream) : _entity_manager(*this), _trait_manager(*this), _render_stream(rstream), _resource_manager(*this), _phy_world(*this)
	{
		_vekt_atlases.reserve(32);
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
		_trait_manager.register_cache<trait_audio, MAX_WORLD_TRAIT_AUDIO>();
		_trait_manager.register_cache<trait_canvas, MAX_WORLD_TRAIT_CANVAS>();

		_phy_world.init();
		_audio_manager.init();

#ifdef SFG_TOOLMODE
		debug_console::get()->register_console_function("start_playmode", [this]() { start_playmode(); });
		debug_console::get()->register_console_function("stop_playmode", [this]() { stop_playmode(); });
		debug_console::get()->register_console_function("start_physics", [this]() { start_physics(); });
		debug_console::get()->register_console_function("stop_physics", [this]() { stop_physics(); });
#endif

		_vekt_fonts.set_callback_user_data(this);
		_vekt_fonts.set_atlas_created_callback(on_atlas_created);
		_vekt_fonts.set_atlas_updated_callback(on_atlas_updated);
		_vekt_fonts.set_atlas_destroyed_callback(on_atlas_destroyed);
	};

	world::~world()
	{
		_audio_manager.uninit();
		_text_allocator.uninit();
		_phy_world.uninit();
		_vekt_fonts.uninit();
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

		_trait_manager.view<trait_canvas>([&](trait_canvas& cnv) -> trait_view_result {
			cnv.draw(*this, res);
			return trait_view_result::cont;
		});
	}

	void world::interpolate(double interpolation)
	{
		_entity_manager.interpolate_entities(interpolation);
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

	resource_handle world::find_atlas_texture(vekt::atlas* atlas)
	{
		auto it = std::find_if(_vekt_atlases.begin(), _vekt_atlases.end(), [atlas](const atlas_data& d) -> bool { return d.atlas == atlas; });
		SFG_ASSERT(it != _vekt_atlases.end());
		return it->handle;
	}

	void world::on_atlas_created(vekt::atlas* atlas, void* user_data)
	{
		world*			w			 = static_cast<world*>(user_data);
		resource_handle atlas_handle = w->_resource_manager.add_resource<texture>(0);
		texture&		t			 = w->_resource_manager.get_resource<texture>(atlas_handle);

		w->_vekt_atlases.push_back({
			.atlas	= atlas,
			.handle = atlas_handle,
		});
	}

	void world::on_atlas_updated(vekt::atlas* atlas, void* user_data)
	{
		world* w  = static_cast<world*>(user_data);
		auto   it = std::find_if(w->_vekt_atlases.begin(), w->_vekt_atlases.end(), [atlas](const atlas_data& d) -> bool { return d.atlas == atlas; });
		SFG_ASSERT(it != w->_vekt_atlases.end());
		const resource_handle handle = it->handle;

		texture& t = w->_resource_manager.get_resource<texture>(handle);
		t.destroy(*w, handle);

		texture_raw	 raw = {};
		const format fmt = format::r8_unorm;

		const size_t sz = atlas->get_width() * atlas->get_height();

		uint8* data = reinterpret_cast<uint8*>(SFG_MALLOC(sz));

		if (data != 0)
			SFG_MEMCPY(data, atlas->get_data(), sz);

		raw.load_from_data(data, vector2ui16(atlas->get_width(), atlas->get_height()), static_cast<uint8>(fmt), false);
		t.create_from_loader(raw, *w, handle);
	}

	void world::on_atlas_destroyed(vekt::atlas* atlas, void* user_data)
	{
		world* w  = static_cast<world*>(user_data);
		auto   it = std::find_if(w->_vekt_atlases.begin(), w->_vekt_atlases.end(), [atlas](const atlas_data& d) -> bool { return d.atlas == atlas; });
		SFG_ASSERT(it != w->_vekt_atlases.end());

		const resource_handle handle = it->handle;
		if (w->_resource_manager.is_valid<texture>(handle))
		{
			texture& t = w->_resource_manager.get_resource<texture>(handle);
			t.destroy(*w, handle);
			w->_resource_manager.remove_resource<texture>(handle);
		}
		w->_vekt_atlases.erase(it);
	}
}
