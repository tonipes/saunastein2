// Copyright (c) 2025 Inan Evin
#include "trait_canvas.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "math/math.hpp"
#include "world/world.hpp"
#include "gui/vekt.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"
#include "resources/material.hpp"
#include "resources/font.hpp"

namespace SFG
{
	void trait_canvas::on_add(world& w)
	{
		w.get_entity_manager().on_add_render_proxy(_header.entity);

		chunk_allocator32& aux = w.get_trait_manager().get_aux();
		_builder			   = new vekt::builder();
	}

	void trait_canvas::on_remove(world& w)
	{
		w.get_entity_manager().on_remove_render_proxy(_header.entity);

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

	void trait_canvas::update_counts_and_init(world& w, uint32 max_widget_count, uint16 max_buffer_count)
	{
		_max_buffer_count = max_buffer_count;
		_max_widget_count = max_widget_count;

		if (_flags.is_set(trait_canvas::flags::is_init))
			uninit_builder(w);
		init_builder(w);
	}

	void trait_canvas::draw(world& w, const vector2ui16& size)
	{
		SFG_ASSERT(_flags.is_set(trait_canvas::flags::is_init));

		_world = &w;

		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::canvas_reset_draws,
		});

		_vtx_counter = _idx_counter = 0;

		const vector2 draw_size = vector2ui16(_size.x == 0 ? size.x : _size.x, _size.y == 0 ? size.y : _size.y);
		_builder->build_begin(draw_size);
		_builder->build_end();
		_builder->flush();
	}

	void trait_canvas::set_is_3d(world& w, uint8 is_3d)
	{
		_flags.set(trait_canvas::flags::is_3d, is_3d);

		if (_flags.is_set(trait_canvas::flags::is_init))
		{
			const render_event_canvas ev = {
				.entity_index = _header.entity.index,
				.is_3d		  = _flags.is_set(trait_canvas::flags::is_3d),
			};

			w.get_render_stream().add_event(
				{
					.index		= _header.own_handle.index,
					.event_type = render_event_type::canvas_update,
				},
				ev);
		}
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
		_flags.set(trait_canvas::flags::is_init);

		const render_event_canvas ev = {
			.entity_index = _header.entity.index,
			.vertex_size  = static_cast<uint32>(cnf.vertex_buffer_sz),
			.index_size	  = static_cast<uint32>(cnf.index_buffer_sz),
			.is_3d		  = _flags.is_set(trait_canvas::flags::is_3d),
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::create_canvas,
			},
			ev);
	}

	void trait_canvas::uninit_builder(world& w)
	{
		_builder->uninit();
		_flags.remove(trait_canvas::flags::is_init);

		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::destroy_canvas,
		});
	}

	void trait_canvas::on_draw(const vekt::draw_buffer& buffer, void* ud)
	{
		trait_canvas* cnv = static_cast<trait_canvas*>(ud);

		render_event_canvas_add_draw ev = {};

		const vector4 clip = buffer.clip;

		if (clip.y < 0.0f)
		{
			ev.clip.y = 0;
			ev.clip.w = static_cast<uint16>(math::max(0.0f, clip.w + clip.y));
		}
		else
		{
			ev.clip.y = static_cast<uint16>(clip.y);
			ev.clip.w = static_cast<uint16>(clip.w);
		}
		if (clip.x < 0.0f)
		{
			ev.clip.x = 0;
			ev.clip.z = math::min((uint16)0, static_cast<uint16>(clip.x + clip.z));
		}
		else
		{
			ev.clip.x = static_cast<uint16>(clip.x);
			ev.clip.z = static_cast<uint16>(clip.z);
		}

		ev.start_index		= cnv->_idx_counter;
		ev.start_vertex		= cnv->_vtx_counter;
		ev.index_count		= buffer.index_count;
		ev.vertex_data		= reinterpret_cast<uint8*>(buffer.vertex_start);
		ev.vertex_data_size = buffer.vertex_count * sizeof(vekt::vertex);
		ev.index_data		= reinterpret_cast<uint8*>(buffer.index_start);
		ev.index_data_size	= buffer.index_count * sizeof(vekt::index);

		cnv->_idx_counter += buffer.index_count;
		cnv->_vtx_counter += buffer.vertex_count;

		void* mat = buffer.user_data;

		resource_handle mat_handle	 = {};
		resource_handle atlas_handle = {};

		if (mat != nullptr)
		{
			mat_handle = *reinterpret_cast<resource_handle*>(mat);
			SFG_ASSERT(!mat || cnv->_world->get_resource_manager().is_valid<material>(mat_handle));
		}
		else
		{
			resource_manager& rm = cnv->_world->get_resource_manager();

			if (buffer.used_font)
			{
				mat_handle		= buffer.used_font->type == vekt::font_type::sdf ? rm.get_default_gui_sdf_mat() : rm.get_default_gui_text_mat();
				atlas_handle	= cnv->_world->find_atlas_texture(buffer.used_font->_atlas);
				ev.atlas_exists = 1;
			}
			else
				mat_handle = rm.get_default_gui_mat();
		}

		ev.material_handle = mat_handle.index;
		ev.atlas_handle	   = atlas_handle.index;

		cnv->_world->get_render_stream().add_event(
			{
				.index		= cnv->_header.own_handle.index,
				.event_type = render_event_type::canvas_add_draw,
			},
			ev);
	}

}