// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/bitmask.hpp"
#include "data/vector.hpp"
#include "memory/text_allocator.hpp"
#include "entity_manager.hpp"
#include "trait_manager.hpp"
#include "gui/vekt.hpp"
#include "resources/resource_manager.hpp"
#include "physics/physics_world.hpp"
#include "audio/audio_manager.hpp"

struct ma_engine;

namespace vekt
{
	class atlas;
}

namespace SFG
{
	struct window_event;
	struct vector2ui16;
	struct world_raw;
	class render_event_stream;

	class world
	{
	public:
		enum flags
		{
			world_flags_is_init			  = 1 << 0,
			world_flags_is_playing		  = 1 << 1,
			world_flags_is_physics_active = 1 << 2,
		};

	public:
		world(render_event_stream& rstream);
		~world();

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void create_from_loader(world_raw& raw);
		void tick(const vector2ui16& res, float dt);
		void interpolate(double interpolation);
		bool on_window_event(const window_event& ev);

		// -----------------------------------------------------------------------------
		// playmode
		// -----------------------------------------------------------------------------
		void start_playmode();
		void stop_playmode();
		void start_physics();
		void stop_physics();

		// -----------------------------------------------------------------------------
		// atlas
		// -----------------------------------------------------------------------------
		resource_handle find_atlas_texture(vekt::atlas* atl);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline physics_world& get_physics_world()
		{
			return _phy_world;
		}

		inline entity_manager& get_entity_manager()
		{
			return _entity_manager;
		}

		inline trait_manager& get_trait_manager()
		{
			return _trait_manager;
		}

		inline bitmask<uint8>& get_flags()
		{
			return _flags;
		}

		inline text_allocator& get_text_allocator()
		{
			return _text_allocator;
		}

		inline vekt::font_manager& get_font_manager()
		{
			return _vekt_fonts;
		}

		inline render_event_stream& get_render_stream()
		{
			return _render_stream;
		}

		inline resource_manager& get_resource_manager()
		{
			return _resource_manager;
		}

		inline audio_manager& get_audio_manager()
		{
			return _audio_manager;
		}

	private:
		// -----------------------------------------------------------------------------
		// vekt
		// -----------------------------------------------------------------------------

		struct atlas_data
		{
			vekt::atlas*	atlas  = nullptr;
			resource_handle handle = {};
		};

		static void on_atlas_created(vekt::atlas* atlas, void* user_data);
		static void on_atlas_updated(vekt::atlas* atlas, void* user_data);
		static void on_atlas_destroyed(vekt::atlas* atlas, void* user_data);

	private:
		audio_manager		 _audio_manager = {};
		vector<atlas_data>	 _vekt_atlases	= {};
		ma_engine*			 _sound_engine	= nullptr;
		render_event_stream& _render_stream;
		entity_manager		 _entity_manager;
		resource_manager	 _resource_manager;
		trait_manager		 _trait_manager;
		physics_world		 _phy_world;
		vekt::font_manager	 _vekt_fonts = {};
		text_allocator		 _text_allocator;
		bitmask<uint8>		 _flags = 0;
	};
}