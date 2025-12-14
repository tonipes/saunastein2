// Copyright (c) 2025 Inan Evin

#include "world.hpp"
#include "world/world_listener.hpp"
#include "game/game_max_defines.hpp"
#include "platform/window.hpp"

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
#include "components/comp_camera.hpp"
#include "components/comp_light.hpp"
#include "components/comp_mesh_instance.hpp"
#include "components/comp_model_instance.hpp"
#include "components/comp_ambient.hpp"
#include "components/comp_physics.hpp"
#include "components/comp_audio.hpp"
#include "components/comp_canvas.hpp"
#include "components/comp_animation_controller.hpp"

#include <tracy/Tracy.hpp>

namespace SFG
{
	world::world(render_event_stream& rstream) : _entity_manager(*this), _comp_manager(*this), _render_stream(rstream), _resource_manager(*this), _phy_world(*this)
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
		_comp_manager.register_cache<comp_camera, MAX_WORLD_COMP_CAMERAS>();
		_comp_manager.register_cache<comp_point_light, MAX_WORLD_COMP_POINT_LIGHTS>();
		_comp_manager.register_cache<comp_spot_light, MAX_WORLD_COMP_SPOT_LIGHTS>();
		_comp_manager.register_cache<comp_dir_light, MAX_WORLD_COMP_DIR_LIGHTS>();
		_comp_manager.register_cache<comp_model_instance, MAX_WORLD_COMP_MODEL_INSTANCES>();
		_comp_manager.register_cache<comp_mesh_instance, MAX_WORLD_COMP_MESH_INSTANCES>();
		_comp_manager.register_cache<comp_ambient, MAX_WORLD_COMP_AMBIENT>();
		_comp_manager.register_cache<comp_physics, MAX_WORLD_COMP_PHYSICS>();
		_comp_manager.register_cache<comp_audio, MAX_WORLD_COMP_AUDIO>();
		_comp_manager.register_cache<comp_canvas, MAX_WORLD_COMP_CANVAS>();
		_comp_manager.register_cache<comp_animation_controller, MAX_WORLD_COMP_ANIMS>();

		_phy_world.init();
		_audio_manager.init();

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
		_comp_manager.init();
		_anim_graph.init();
	}

	void world::uninit()
	{
		_comp_manager.uninit();
		_entity_manager.uninit();
		_resource_manager.uninit();
		_anim_graph.uninit();
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
		ZoneScoped;

		_anim_graph.tick(*this, dt);
		_resource_manager.tick();

		if (_play_mode != play_mode::none)
			_phy_world.simulate(dt);

		_comp_manager.view<comp_canvas>([&](comp_canvas& cnv) -> comp_view_result {
			cnv.draw(*this, res);
			return comp_view_result::cont;
		});
	}

	void world::calculate_abs_transforms()
	{
		ZoneScoped;
		_entity_manager.calculate_abs_transforms();
	}

	void world::interpolate(double interpolation)
	{
		ZoneScoped;
		_entity_manager.interpolate_entities(interpolation);
	}

	bool world::on_window_event(const window_event& ev, window* wnd)
	{
		if (!_flags.is_set(world_flags_is_init))
			return false;

		bool handled = false;

		_comp_manager.view<comp_canvas>([&](comp_canvas& cnv) -> comp_view_result {
			vekt::builder* builder = cnv.get_builder();

			const world_handle entity_handle = cnv.get_header().entity;
			const bool		   is_invisible	 = _entity_manager.get_entity_flags(entity_handle).is_set(entity_flags::entity_flags_invisible);
			if (is_invisible)
				return comp_view_result::cont;

			const vekt::input_event_type ev_type = ev.sub_type == window_event_sub_type::press ? vekt::input_event_type::pressed : vekt::input_event_type::released;

			if (ev.type == window_event_type::key)
			{
				const vekt::input_event_result res = builder->on_key_event({
					.type	   = ev_type,
					.key	   = ev.button,
					.scan_code = ev.value.x,
				});

				handled = res == vekt::input_event_result::handled;
			}
			else if (ev.type == window_event_type::mouse)
			{
				const vekt::input_event_result res = builder->on_mouse_event({
					.type	  = ev_type,
					.button	  = ev.button,
					.position = vector2(ev.value.x, ev.value.y),
				});

				handled = res == vekt::input_event_result::handled;
			}
			else if (ev.type == window_event_type::wheel)
			{
				const vekt::input_event_result res = builder->on_mouse_wheel_event({
					.amount = static_cast<float>(ev.value.x),
				});

				handled = res == vekt::input_event_result::handled;
			}
			else if (ev.type == window_event_type::delta)
			{
				const vector2i16& mp = wnd->get_mouse_position();
				builder->on_mouse_move(vector2(mp.x, mp.y));
			}

			return handled ? comp_view_result::stop : comp_view_result::cont;
		});

		return handled;
	}

	void world::set_playmode(play_mode mode)
	{
		if (mode == _play_mode)
			return;

		if (mode == play_mode::none && _play_mode == play_mode::physics_only)
		{
			_phy_world.uninit_simulation();

			if (_listener)
				_listener->on_stopped_physics();
		}
		else if (mode == play_mode::none && _play_mode == play_mode::full)
		{
			_phy_world.uninit_simulation();
			// stop playing

			if (_listener)
				_listener->on_stopped_play();
		}
		else if (mode == play_mode::physics_only && _play_mode == play_mode::full)
		{
			SFG_ERR("Can't switch to physics playmode from full mode.");
			return;
		}
		else if (mode == play_mode::full && _play_mode == play_mode::physics_only)
		{
			SFG_ERR("Can't switch to full playmode from physics only mode.");
			return;
		}
		else if (mode == play_mode::physics_only && _play_mode == play_mode::none)
		{
			_phy_world.init_simulation();

			if (_listener)
				_listener->on_started_physics();
		}
		else if (mode == play_mode::full && _play_mode == play_mode::none)
		{
			_phy_world.init_simulation();
			// start playing

			if (_listener)
				_listener->on_started_play();
		}

		_play_mode = mode;
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
