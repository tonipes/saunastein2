// Copyright (c) 2025 Inan Evin
#include "trait_canvas.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "world/world.hpp"
#include "gui/vekt.hpp"

namespace SFG
{
	void trait_canvas::on_add(world& w)
	{
		_builder = new vekt::builder();
	}

	void trait_canvas::on_remove(world& w)
	{
		if (_flags.is_set(trait_canvas::flags::is_init))
			uninit_builder(w);

		delete _builder;
		_builder = nullptr;
	}

	void trait_canvas::serialize(ostream& stream, world& w) const
	{
	}

	void trait_canvas::deserialize(istream& stream, world& w)
	{
	}

	void trait_canvas::set_counts(world& w, uint32 max_widget_count, uint16 max_buffer_count)
	{
		_max_buffer_count = max_buffer_count;
		_max_widget_count = max_widget_count;

		if (_flags.is_set(trait_canvas::flags::is_init))
			uninit_builder(w);

		init_builder(w);
	}

	void trait_canvas::draw(const vector2ui16& size)
	{
		SFG_ASSERT(_flags.is_set(trait_canvas::flags::is_init));

		const vector2 draw_size = vector2ui16(_size.x == 0 ? size.x : _size.x, _size.y == 0 ? size.y : _size.y);
		_builder->build_begin(draw_size);
		_builder->build_end();
		_builder->flush();
	}

	void trait_canvas::init_builder(world& w)
	{
		const vekt::builder::init_config cnf = {
			.widget_count				 = _max_widget_count,
			.vertex_buffer_sz			 = _max_widget_count * 1024,
			.index_buffer_sz			 = _max_widget_count * 1024,
			.text_cache_vertex_buffer_sz = _max_widget_count * 1024,
			.text_cache_index_buffer_sz	 = _max_widget_count * 1024,
			.buffer_count				 = _max_buffer_count,
		};

		_builder->init(cnf);
		_builder->set_on_draw(on_draw, this);
		_vertices.init(cnf.vertex_buffer_sz);
		_indices.init(cnf.index_buffer_sz);
		_flags.set(trait_canvas::flags::is_init);
	}

	void trait_canvas::uninit_builder(world& w)
	{
		_vertices.uninit();
		_indices.uninit();
		_builder->uninit();
		_flags.remove(trait_canvas::flags::is_init);
	}

	void trait_canvas::on_draw(const vekt::draw_buffer& buffer, void* ud)
	{
		trait_canvas* cnv = static_cast<trait_canvas*>(ud);
	}

}