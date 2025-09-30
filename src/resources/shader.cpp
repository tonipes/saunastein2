// Copyright (c) 2025 Inan Evin

#include "shader.hpp"
#include "shader_raw.hpp"
#include "data/string.hpp"
#include "data/vector.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/renderer.hpp"
#include "world/world.hpp"
#include "reflection/reflection.hpp"

namespace SFG
{
	shader_reflection::shader_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<shader>::value, "stkphy");

#ifdef SFG_TOOLMODE

		m.add_function<void*, const char*>("cook_from_file"_hs, [](const char* path) -> void* {
			shader_raw* raw = new shader_raw();
			if (!raw->cook_from_file(path))
			{
				delete raw;
				return nullptr;
			}

			return raw;
		});
#endif

		m.add_function<void*, istream&>("cook_from_stream"_hs, [](istream& stream) -> void* {
			shader_raw* raw = new shader_raw();
			raw->deserialize(stream);
			return raw;
		});

		m.add_function<resource_handle, void*, world&, string_id>("create_from_raw"_hs, [](void* raw, world& w, string_id sid) -> resource_handle {
			shader_raw*		 raw_ptr   = reinterpret_cast<shader_raw*>(raw);
			world_resources& resources = w.get_resources();
			resource_handle	 handle	   = resources.add_resource<shader>(sid);
			shader&			 res	   = resources.get_resource<shader>(handle);
			res.create_from_raw(*raw_ptr, false, renderer::get_bind_layout_global());
			delete raw_ptr;
			return handle;
		});

		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<shader>(MAX_WORLD_SHADERS); });

		m.add_function<void, world&, resource_handle>("destroy"_hs, [](world& w, resource_handle h) -> void {
			world_resources& res = w.get_resources();
			res.get_resource<shader>(h).destroy();
			res.remove_resource<shader>(h);
		});

		m.add_function<void, void*, ostream&>("serialize"_hs, [](void* loader, ostream& stream) -> void {
			shader_raw* raw = reinterpret_cast<shader_raw*>(loader);
			raw->serialize(stream);
		});
	}

	shader::~shader()
	{
		SFG_ASSERT(!_flags.is_set(shader::flags::hw_exists));
	}

	void shader::create_from_raw(const shader_raw& raw, bool use_embedded_layout, gfx_id layout)
	{
		SFG_ASSERT(!_flags.is_set(shader::flags::hw_exists));
		gfx_backend* backend = gfx_backend::get();

		shader_desc desc = raw.desc;

		if (use_embedded_layout)
			desc.flags.set(shader_flags::shf_use_embedded_layout);
		else
			desc.layout = layout;

		_hw = backend->create_shader(desc);
		_flags.set(shader::flags::hw_exists);
		_flags.set(shader::flags::is_skinned, raw.is_skinned);
	}

	void shader::push_create_event(render_event_stream& stream, resource_handle handle)
	{
	}

	void shader::destroy()
	{
		SFG_ASSERT(_flags.is_set(shader::flags::hw_exists));
		gfx_backend::get()->destroy_shader(_hw);
		_hw = 0;
		_flags.remove(shader::flags::hw_exists);
	}

	gfx_id shader::get_hw() const
	{
		return _hw;
	}

} // namespace Lina
