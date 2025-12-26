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

#include "data/bitmask.hpp"
#include "data/vector.hpp"
#include "memory/text_allocator.hpp"

// world
#include "world/entity_manager.hpp"
#include "world/component_manager.hpp"
#include "world/common_world.hpp"
#include "world/time_manager.hpp"

#include "gui/vekt.hpp"
#include "resources/resource_manager.hpp"
#include "physics/physics_world.hpp"
#include "audio/audio_manager.hpp"

// animation
#include "animation/animation_graph.hpp"
#include "animation/bone_manager.hpp"

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
	class window;
	class world_listener;

	class world
	{
	public:
		enum flags
		{
			world_flags_is_init = 1 << 0,
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
		void calculate_abs_transforms();
		void interpolate(double interpolation);
		bool on_window_event(const window_event& ev, window* wnd);

		// -----------------------------------------------------------------------------
		// playmode
		// -----------------------------------------------------------------------------
		void set_playmode(play_mode mode);

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

		inline component_manager& get_comp_manager()
		{
			return _comp_manager;
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

		inline animation_graph& get_animation_graph()
		{
			return _anim_graph;
		}

		inline void set_listener(world_listener* list)
		{
			_listener = list;
		}

		inline time_manager& get_time_manager()
		{
			return _time_manager;
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
		resource_manager   _resource_manager;
		entity_manager	   _entity_manager;
		physics_world	   _phy_world;
		component_manager  _comp_manager;
		text_allocator	   _text_allocator;
		audio_manager	   _audio_manager = {};
		animation_graph	   _anim_graph	  = {};
		vekt::font_manager _vekt_fonts	  = {};
		time_manager	   _time_manager  = {};
		bone_manager	   _bone_manager  = {};

		vector<atlas_data>	 _vekt_atlases = {};
		ma_engine*			 _sound_engine = nullptr;
		world_listener*		 _listener	   = nullptr;
		render_event_stream& _render_stream;

		bitmask<uint8> _flags	  = 0;
		play_mode	   _play_mode = play_mode::none;
	};
}