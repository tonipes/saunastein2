// Copyright (c) 2025 Inan Evin

#pragma once
#include "data/bitmask.hpp"
#include "common_world.hpp"
#include "data/vector.hpp"
#include "memory/text_allocator.hpp"
#include "entity_manager.hpp"
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

		void init();
		void uninit();
		void create_from_loader(world_raw& raw);

		void tick(const vector2ui16& res, float dt);
		void post_tick(double interpolation);
		void pre_render(const vector2ui16& res);

		void load_debug();

#ifdef SFG_TOOLMODE
		void save(const char* path);
		void load(const char* path);
#endif
		bool on_window_event(const window_event& ev);

		inline entity_manager& get_entity_manager()
		{
			return _entity_manager;
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
		text_allocator		 _text_allocator;
		bitmask<uint8>		 _flags = 0;
		entity_manager		 _entity_manager;
		resource_manager	 _resource_manager;
		vekt::font_manager	 _vekt_fonts = {};
	};
}