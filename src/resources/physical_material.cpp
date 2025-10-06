// Copyright (c) 2025 Inan Evin

#include "physical_material.hpp"
#include "physical_material_raw.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"

namespace SFG
{
	physical_material_reflection::physical_material_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<physical_material>::value, "stkphy");

#ifdef SFG_TOOLMODE

		m.add_function<void*, const char*>("cook_from_file"_hs, [](const char* path) -> void* {
			physical_material_raw* raw = new physical_material_raw();
			if (!raw->cook_from_file(path))
			{
				delete raw;
				return nullptr;
			}

			return raw;
		});
#endif

		m.add_function<void*, istream&>("cook_from_stream"_hs, [](istream& stream) -> void* {
			physical_material_raw* raw = new physical_material_raw();
			raw->deserialize(stream);
			return raw;
		});

		m.add_function<resource_handle, void*, world&, string_id>("create_from_raw"_hs, [](void* raw, world& w, string_id sid) -> resource_handle {
			physical_material_raw* raw_ptr	 = reinterpret_cast<physical_material_raw*>(raw);
			world_resources&	   resources = w.get_resources();
			resource_handle		   handle	 = resources.add_resource<physical_material>(sid);
			physical_material&	   res		 = resources.get_resource<physical_material>(handle);
			res.create_from_raw(*raw_ptr, resources.get_aux());
			delete raw_ptr;
			return handle;
		});

		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<physical_material>(MAX_WORLD_PHYSICAL_MATERIALS); });

		m.add_function<void, world&, resource_handle>("destroy"_hs, [](world& w, resource_handle h) -> void {
			world_resources& res = w.get_resources();
			res.get_resource<physical_material>(h).destroy(res.get_aux());
			res.remove_resource<physical_material>(h);
		});

		m.add_function<void, void*, ostream&>("serialize"_hs, [](void* loader, ostream& stream) -> void {
			physical_material_raw* raw = reinterpret_cast<physical_material_raw*>(loader);
			raw->serialize(stream);
		});
	}

	void physical_material::create_from_raw(const physical_material_raw& raw, chunk_allocator32& alloc)
	{
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
	}

	void physical_material::destroy(chunk_allocator32& alloc)
	{
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
	}
}
