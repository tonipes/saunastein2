// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "memory/pool_handle.hpp"
#include "reflection/reflection.hpp"
#include "common/type_id.hpp"
#include "data/istream.hpp"
#include "data/ostream.hpp"
#include "world/world.hpp"

namespace SFG
{
	template <typename T, typename U> class resource_reflection_template
	{
	public:
		resource_reflection_template() = delete;

		resource_reflection_template(const char* extension, int count, string_id creation_function)
		{
			meta& m = reflection::get().register_meta(type_id<T>::value, extension);

#ifdef SFG_TOOLMODE

			m.add_function<void*, const char*>("cook_from_file"_hs, [](const char* path) -> void* {
				U* raw = new U();
				if (!raw->cook_from_file(path))
				{
					delete raw;
					return nullptr;
				}

				return raw;
			});
#endif

			m.add_function<void*, istream&>("cook_from_stream"_hs, [](istream& stream) -> void* {
				U* raw = new U();
				raw->deserialize(stream);
				return raw;
			});

			m.add_function<resource_handle, void*, world&, string_id>(creation_function, [](void* raw, world& w, string_id sid) -> resource_handle {
				U*				 raw_ptr   = reinterpret_cast<U*>(raw);
				world_resources& resources = w.get_resources();
				resource_handle	 handle	   = resources.create_resource<T>(sid);
				T&				 res	   = resources.get_resource<T>(handle);
				res.create_from_raw(*raw_ptr, w);
				delete raw_ptr;
				return handle;
			});

			m.add_function<void, world&>("init_resource_storage"_hs, [count](world& w) -> void { w.get_resources().init_storage<T>(count); });

			m.add_function<void, world&, resource_handle>("destroy"_hs, [](world& w, resource_handle h) -> void {
				world_resources& res = w.get_resources();
				res.get_resource<T>(h).destroy(w);
			});

			m.add_function<void, void*, ostream&>("serialize"_hs, [](void* loader, ostream& stream) -> void {
				U* raw = reinterpret_cast<U*>(loader);
				raw->serialize(stream);
			});
		}
	};

	template <typename T> class resource_reflection_template_light
	{
	public:
		resource_reflection_template_light() = delete;
		resource_reflection_template_light(int count)
		{
			meta& m = reflection::get().register_meta(type_id<T>::value, "");
			m.add_function<void, world&>("init_resource_storage"_hs, [count](world& w) -> void { w.get_resources().init_storage<T>(count); });
		}
	};

}
