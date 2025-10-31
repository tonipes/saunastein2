// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/bitmask.hpp"
#include "memory/text_allocator.hpp"
#include "entity_manager.hpp"
#include "trait_manager.hpp"
#include "gui/vekt.hpp"
#include "resources/resource_manager.hpp"

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
			world_flags_is_init	   = 1 << 0,
			world_flags_is_playing = 1 << 1,
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
		void post_tick(double interpolation);
		bool on_window_event(const window_event& ev);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

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

	private:
		render_event_stream& _render_stream;
		entity_manager		 _entity_manager;
		resource_manager	 _resource_manager;
		trait_manager		 _trait_manager;
		vekt::font_manager	 _vekt_fonts = {};
		text_allocator		 _text_allocator;
		bitmask<uint8>		 _flags = 0;
	};
}