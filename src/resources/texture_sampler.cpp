// Copyright (c) 2025 Inan Evin

#include "texture_sampler.hpp"
#include "texture_sampler_raw.hpp"
#include "math/math_common.hpp"
#include "io/assert.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "resource_reflection_template.hpp"
#include "world/world.hpp"

namespace SFG
{
	texture_sampler_reflection::texture_sampler_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<texture_sampler>::value, "stkphy");

#ifdef SFG_TOOLMODE

		m.add_function<void*, const char*>("cook_from_file"_hs, [](const char* path) -> void* {
			texture_sampler_raw* raw = new texture_sampler_raw();
			if (!raw->cook_from_file(path))
			{
				delete raw;
				return nullptr;
			}

			return raw;
		});
#endif

		m.add_function<void*, istream&>("cook_from_stream"_hs, [](istream& stream) -> void* {
			texture_sampler_raw* raw = new texture_sampler_raw();
			raw->deserialize(stream);
			return raw;
		});

		m.add_function<resource_handle, void*, world&, string_id>("create_from_raw"_hs, [](void* raw, world& w, string_id sid) -> resource_handle {
			texture_sampler_raw* raw_ptr   = reinterpret_cast<texture_sampler_raw*>(raw);
			world_resources&	 resources = w.get_resources();
			resource_handle		 handle	   = resources.add_resource<texture_sampler>(sid);
			texture_sampler&	 res	   = resources.get_resource<texture_sampler>(handle);
			res.create_from_raw(*raw_ptr);
			delete raw_ptr;
			return handle;
		});

		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<texture_sampler>(MAX_WORLD_SAMPLERS); });

		m.add_function<void, world&, resource_handle>("destroy"_hs, [](world& w, resource_handle h) -> void {
			world_resources& res = w.get_resources();
			res.get_resource<texture_sampler>(h).destroy();
			res.remove_resource<texture_sampler>(h);
		});

		m.add_function<void, void*, ostream&>("serialize"_hs, [](void* loader, ostream& stream) -> void {
			texture_sampler_raw* raw = reinterpret_cast<texture_sampler_raw*>(loader);
			raw->serialize(stream);
		});
	}

	texture_sampler::~texture_sampler()
	{
		SFG_ASSERT(_hw == std::numeric_limits<gfx_id>::max());
	}

	void texture_sampler::create_from_raw(const texture_sampler_raw& raw)
	{
		gfx_backend* backend = gfx_backend::get();
		_hw					 = backend->create_sampler(raw.desc);
	}

	void texture_sampler::destroy()
	{
		SFG_ASSERT(_hw != std::numeric_limits<gfx_id>::max());
		gfx_backend* backend = gfx_backend::get();
		backend->destroy_sampler(_hw);
		_hw = std::numeric_limits<gfx_id>::max();
	}

}
