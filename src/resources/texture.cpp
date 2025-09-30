// Copyright (c) 2025 Inan Evin

#include "texture.hpp"
#include "texture_raw.hpp"
#include "memory/memory.hpp"
#include "memory/memory_tracer.hpp"
#include "io/log.hpp"
#include "io/assert.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/world/world_renderer.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"

namespace SFG
{
	texture_reflection g_texture_reflection = {};

	texture_reflection::texture_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<texture>::value, "stkphy");

#ifdef SFG_TOOLMODE

		m.add_function<void*, const char*>("cook_from_file"_hs, [](const char* path) -> void* {
			texture_raw* raw = new texture_raw();
			if (!raw->cook_from_file(path))
			{
				delete raw;
				return nullptr;
			}

			return raw;
		});
#endif

		m.add_function<void*, istream&>("cook_from_stream"_hs, [](istream& stream) -> void* {
			texture_raw* raw = new texture_raw();
			raw->deserialize(stream);
			return raw;
		});

		m.add_function<resource_handle, void*, world&, string_id>("create_from_raw"_hs, [](void* raw, world& w, string_id sid) -> resource_handle {
			texture_raw*	 raw_ptr   = reinterpret_cast<texture_raw*>(raw);
			world_resources& resources = w.get_resources();
			resource_handle	 handle	   = resources.add_resource<texture>(sid);
			texture&		 res	   = resources.get_resource<texture>(handle);
			res.create_from_raw(*raw_ptr);

			resources.add_pending_resource_event({
				.res		= &res,
				.handle		= handle,
				.type_id	= type_id<texture>::value,
				.is_destroy = 0,
			});

			delete raw_ptr;

			return handle;
		});

		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<texture>(MAX_WORLD_TEXTURES); });

		m.add_function<void, world&, resource_handle>("destroy"_hs, [](world& w, resource_handle h) -> void {
			world_resources& resources = w.get_resources();
			texture&		 txt	   = resources.get_resource<texture>(h);

			resources.add_pending_resource_event({
				.res		= &txt,
				.handle		= h,
				.type_id	= type_id<texture>::value,
				.is_destroy = 0,
			});

			resources.remove_resource<texture>(h);
		});

		m.add_function<void, void*, ostream&>("serialize"_hs, [](void* loader, ostream& stream) -> void {
			texture_raw* raw = reinterpret_cast<texture_raw*>(loader);
			raw->serialize(stream);
		});
	}

	texture::~texture()
	{
		SFG_ASSERT(_cpu_buffers.empty(), "");
		SFG_ASSERT(!_flags.is_set(texture::flags::hw_exists));
	}

	void texture::create_from_raw(const texture_raw& raw)
	{
		_cpu_buffers	= raw.buffers;
		_texture_format = raw.texture_format;

		const size_t sz = raw.name.size() > NAME_SIZE ? NAME_SIZE - 1 : raw.name.size();
		SFG_MEMCPY(_name, raw.name.data(), sz);

		if (sz != raw.name.size())
		{
			char nt = '\0';
			SFG_MEMCPY(_name + sz - 1, &nt, 1);
		}

		SFG_ASSERT(_cpu_buffers.empty());
	}

	void texture::push_create_event(render_event_stream& stream, resource_handle handle)
	{
		const vector2ui16 size = vector2ui16(get_width(), get_height());
		const format	  fmt  = static_cast<format>(_texture_format);

		stream.add_event({.create_callback =
							  [size, fmt, buffers = this->_cpu_buffers, name = this->_name](void* data_storage) {
								  gfx_backend* backend = gfx_backend::get();
								  const gfx_id handle  = backend->create_texture({
									   .texture_format = fmt,
									   .size		   = size,
									   .flags		   = texture_flags::tf_is_2d | texture_flags::tf_sampled,
									   .views		   = {{}},
									   .mip_levels	   = static_cast<uint8>(buffers.size()),
									   .array_length   = 1,
									   .samples		   = 1,
									   .debug_name	   = name,
								   });

								  uint32 total_size = 0;
								  for (const texture_buffer& buf : buffers)
									  total_size += backend->get_texture_size(buf.size.x, buf.size.y, buf.bpp);

								  const uint32 intermediate_size = backend->align_texture_size(total_size);

								  const gfx_id intermediate = backend->create_resource({
									  .size		  = intermediate_size,
									  .flags	  = resource_flags::rf_cpu_visible,
									  .debug_name = "texture_intermediate",
								  });

								  texture_data_storage* data = reinterpret_cast<texture_data_storage*>(data_storage);
								  data->buffers				 = buffers;
								  data->hw					 = handle;
								  data->intermediate_buffer	 = intermediate;
							  },
						  .destroy_callback =
							  [](void* data_storage) {
								  texture_data_storage* data = reinterpret_cast<texture_data_storage*>(data_storage);

								  gfx_backend* backend = gfx_backend::get();
								  backend->destroy_texture(data->hw);
								  backend->destroy_resource(data->intermediate_buffer);
							  },
						  .handle	  = handle,
						  .event_type = render_event_type::render_event_create_texture});

		_cpu_buffers.clear();
	}
	uint8 texture::get_bpp() const
	{
		SFG_ASSERT(!_cpu_buffers.empty());
		return _cpu_buffers[0].bpp;
	}

	uint16 texture::get_width() const
	{
		SFG_ASSERT(!_cpu_buffers.empty());
		return _cpu_buffers[0].size.x;
	}

	uint16 texture::get_height() const
	{
		SFG_ASSERT(!_cpu_buffers.empty());
		return _cpu_buffers[0].size.y;
	}

	gfx_id texture::get_hw() const
	{
		SFG_ASSERT(_flags.is_set(texture::flags::hw_exists));
		return _hw;
	}

}