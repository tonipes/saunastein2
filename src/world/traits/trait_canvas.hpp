// Copyright (c) 2025 Inan Evin
#pragma once

#include "data/bitmask.hpp"
#include "world/traits/common_trait.hpp"
#include "reflection/trait_reflection.hpp"
#include "memory/double_buffered_swap.hpp"
#include "resources/common_resources.hpp"
#include "math/vector2ui16.hpp"

namespace vekt
{
	class builder;
	struct draw_buffer;
}

namespace SFG
{
	class ostream;
	class istream;
	class world;

	class trait_canvas
	{
	public:
		enum flags
		{
			is_init = 1 << 0,
			is_3d	= 1 << 2,
		};
		// -----------------------------------------------------------------------------
		// trait
		// -----------------------------------------------------------------------------

		void on_add(world& w);
		void on_remove(world& w);
		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_counts(world& w, uint32 max_widget_count, uint16 max_buffer_count);
		void draw(const vector2ui16& size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline void set_is_3d(uint8 is_3d)
		{
			_flags.set(trait_canvas::flags::is_3d, is_3d);
		}

		inline const bitmask<uint8>& get_flags() const
		{
			return _flags;
		}

		inline const trait_header& get_header() const
		{
			return _header;
		}

		inline vekt::builder* get_builder() const
		{
			return _builder;
		}

		inline const double_buffered_swap& get_vertices() const
		{
			return _vertices;
		}

		inline const double_buffered_swap& get_indices() const
		{
			return _indices;
		}

		// 0,0 for screen resolution
		inline void set_size(const vector2ui16& sz)
		{
			_size = sz;
		}

	private:
		void		init_builder(world& w);
		void		uninit_builder(world& w);
		static void on_draw(const vekt::draw_buffer& buffer, void* ud);

	private:
		template <typename T, int> friend class trait_cache;

	private:
		trait_header		 _header		   = {};
		vekt::builder*		 _builder		   = nullptr;
		double_buffered_swap _vertices		   = {};
		double_buffered_swap _indices		   = {};
		vector2ui16			 _size			   = vector2ui16::zero;
		uint32				 _max_widget_count = 1024;
		uint16				 _max_buffer_count = 50;
		bitmask<uint8>		 _flags			   = 0;
	};

	REGISTER_TRAIT(trait_canvas);
}
