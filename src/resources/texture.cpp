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
			resource_handle	 handle	   = resources.create_resource<texture>(sid);
			texture&		 res	   = resources.get_resource<texture>(handle);
			res.create_from_raw(*raw_ptr);
			delete raw_ptr;

			w.get_renderer()->get_resource_uploads().add_pending_texture(&res);
			return handle;
		});

		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<texture>(MAX_WORLD_TEXTURES); });

		m.add_function<void, world&, resource_handle>("destroy"_hs, [](world& w, resource_handle h) -> void {
			world_resources& res = w.get_resources();
			res.get_resource<texture>(h).destroy();
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
		_cpu_buffers = raw.buffers;

		SFG_ASSERT(_cpu_buffers.empty());

		gfx_backend* backend = gfx_backend::get();
		_hw					 = backend->create_texture({
							 .texture_format = static_cast<format>(raw.texture_format),
							 .size			 = vector2ui16(get_width(), get_height()),
							 .flags			 = texture_flags::tf_is_2d | texture_flags::tf_sampled,
							 .views			 = {{}},
							 .mip_levels	 = static_cast<uint8>(_cpu_buffers.size()),
							 .array_length	 = 1,
							 .samples		 = 1,
							 .debug_name	 = raw.name.c_str(),

		 });

		_flags.set(texture::flags::hw_exists);

		create_intermediate();
	}

	void texture::destroy_cpu()
	{
		for (texture_buffer& buf : _cpu_buffers)
		{
			PUSH_DEALLOCATION_SZ(buf.size.x * buf.size.y * buf.bpp);
			SFG_FREE(buf.pixels);
		}
		_cpu_buffers.clear();

		if (_flags.is_set(texture::flags::intermediate_exists))
			destroy_intermediate();
	}

	void texture::destroy()
	{
		destroy_cpu();

		SFG_ASSERT(_flags.is_set(texture::flags::hw_exists));
		_flags.remove(texture::flags::hw_exists);
		gfx_backend* backend = gfx_backend::get();
		backend->destroy_texture(_hw);
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

	void texture::create_intermediate()
	{
		SFG_ASSERT(!_flags.is_set(texture::flags::intermediate_exists));
		gfx_backend* backend = gfx_backend::get();

		uint32 total_size = 0;
		for (const texture_buffer& buf : _cpu_buffers)
		{
			total_size += backend->get_texture_size(buf.size.x, buf.size.y, buf.bpp);
		}

		const uint32 intermediate_size = backend->align_texture_size(total_size);

		_intermediate = backend->create_resource({
			.size		= intermediate_size,
			.flags		= resource_flags::rf_cpu_visible,
			.debug_name = "intermediate_texture_res",
		});

		_flags.set(texture::flags::intermediate_exists);
	}

	void texture::destroy_intermediate()
	{
		SFG_ASSERT(_flags.is_set(texture::flags::intermediate_exists));
		gfx_backend* backend = gfx_backend::get();
		backend->destroy_resource(_intermediate);
		_flags.set(texture::flags::intermediate_exists, false);
	}

}