// Copyright (c) 2025 Inan Evin

#pragma once
#include "data/bitmask.hpp"
#include "common_world.hpp"
#include "data/vector.hpp"
#include "memory/text_allocator.hpp"
#include "world/world_resources.hpp"
#include "entity_manager.hpp"
#include "gui/vekt.hpp"
#include <resources/world_raw.hpp>

namespace SFG
{
	struct window_event;
	struct vector2ui16;
	struct world_raw;

	class world_renderer;
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
		void create_from_raw(world_raw& raw);

		void tick(const vector2ui16& res, float dt);
		void post_tick(double interpolation);
		void pre_render(const vector2ui16& res);

		world_handle add_model_to_world(resource_handle model, resource_handle* materials, uint32 material_size);

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

		inline world_resources& get_resources()
		{
			return _resources;
		}

		inline text_allocator& get_text_allocator()
		{
			return _text_allocator;
		}

		inline world_renderer* get_renderer() const
		{
			return _world_renderer;
		}

		inline void set_world_renderer(world_renderer* wr)
		{
			_world_renderer = wr;
		}

		inline vekt::font_manager& get_font_manager()
		{
			return _vekt_fonts;
		}

		inline render_event_stream& get_render_stream()
		{
			return _render_stream;
		}

	private:
		render_event_stream& _render_stream;
		world_renderer*		 _world_renderer = nullptr;
		world_resources		 _resources;
		text_allocator		 _text_allocator;
		bitmask<uint8>		 _flags = 0;
		entity_manager		 _entity_manager;
		vekt::font_manager	 _vekt_fonts = {};
	};
}