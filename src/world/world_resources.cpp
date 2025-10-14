// Copyright (c) 2025 Inan Evin

#include "world_resources.hpp"
#include "gfx/world/world_renderer.hpp"
#include "gfx/renderer.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "app/debug_console.hpp"
#include "project/engine_data.hpp"
#include "world/world.hpp"
#include "world/common_world.hpp"
#include "data/istream.hpp"
#include "data/ostream.hpp"
#include "reflection/reflection.hpp"
#include "data/pair.hpp"
#include "resources/texture_raw.hpp"
#include "resources/texture.hpp"

#include <algorithm>
#include <execution>

#ifdef SFG_TOOLMODE
#include "io/file_system.hpp"
#include "serialization/serialization.hpp"
#endif
namespace SFG
{
	world_resources::world_resources(world& w) : _world(w)
	{
		_aux_memory.init(MAX_WORLD_RESOURCES_AUX_MEMORY);

		const auto& metas = reflection::get().get_metas();
		for (const auto& [sid, meta] : metas)
		{
			if (meta.has_function("init_resource_storage"_hs))
			{
				meta.invoke_function<void, world&>("init_resource_storage"_hs, _world);
			}
		}
	}

	world_resources::~world_resources()
	{
		for (resource_storage& stg : _storages)
		{
			if (stg.storage.get_raw())
				stg.storage.uninit();

			stg.by_hashes.clear();
		}

		_aux_memory.uninit();
	}

	void world_resources::init()
	{
		_file_watch.set_callback(std::bind(&world_resources::on_watched_resource_modified, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		_file_watch.reserve(250);
		_file_watch.set_tick_interval(15);

		// Dummy textures.
		{
			texture_raw dummy_color_raw	 = {};
			texture_raw dummy_orm_raw	 = {};
			texture_raw dummy_normal_raw = {};
			dummy_color_raw.name		 = "dummy_color";
			dummy_normal_raw.name		 = "dummy_normal";
			dummy_orm_raw.name			 = "dummy_orm";
			uint8* dummy_color_data		 = reinterpret_cast<uint8*>(SFG_MALLOC(4));
			uint8* dummy_normal_data	 = reinterpret_cast<uint8*>(SFG_MALLOC(4));
			uint8* dummy_orm_data		 = reinterpret_cast<uint8*>(SFG_MALLOC(4));

			uint8 color_data[4]	 = {255, 255, 255, 255};
			uint8 normal_data[4] = {128, 128, 255, 255};
			uint8 orm_data[4]	 = {255, 255, 0, 255};
			SFG_MEMCPY(dummy_color_data, color_data, 4);
			SFG_MEMCPY(dummy_normal_data, normal_data, 4);
			SFG_MEMCPY(dummy_orm_data, orm_data, 4);

			dummy_color_raw.cook_from_data(dummy_color_data, vector2ui16(1, 1), static_cast<uint8>(format::r8g8b8a8_srgb), false);
			dummy_normal_raw.cook_from_data(dummy_normal_data, vector2ui16(1, 1), static_cast<uint8>(format::r8g8b8a8_unorm), false);
			dummy_orm_raw.cook_from_data(dummy_orm_data, vector2ui16(1, 1), static_cast<uint8>(format::r8g8b8a8_unorm), false);

			_dummy_color_texture	  = add_resource<texture>(DUMMY_COLOR_TEXTURE_SID);
			_dummy_orm_texture		  = add_resource<texture>(DUMMY_ORM_TEXTURE_SID);
			_dummy_normal_texture	  = add_resource<texture>(DUMMY_NORMAL_TEXTURE_SID);
			texture& dummy_color_txt  = get_resource<texture>(_dummy_color_texture);
			texture& dummy_orm_txt	  = get_resource<texture>(_dummy_orm_texture);
			texture& dummy_normal_txt = get_resource<texture>(_dummy_normal_texture);
			dummy_color_txt.create_from_raw(dummy_color_raw, _world.get_render_stream(), _aux_memory, _dummy_color_texture);
			dummy_normal_txt.create_from_raw(dummy_normal_raw, _world.get_render_stream(), _aux_memory, _dummy_normal_texture);
			dummy_orm_txt.create_from_raw(dummy_orm_raw, _world.get_render_stream(), _aux_memory, _dummy_orm_texture);
		}
	}

	void world_resources::uninit()
	{
		for (resource_storage& stg : _storages)
		{
			const string_id tid = stg.type_id;

			for (resource_handle h : stg.storage)
			{
				meta& reflection_meta = reflection::get().resolve(tid);

				if (reflection_meta.has_function("destroy"_hs))
					reflection_meta.invoke_function<void, world&, resource_handle>("destroy"_hs, _world, h);
			}

			stg.storage.reset();
		}

		_aux_memory.reset();
		_file_watch.clear();
	}

	void world_resources::tick()
	{
#ifdef SFG_TOOLMODE
		_file_watch.tick();
#endif
	}

#ifdef SFG_TOOLMODE
	void world_resources::load_resources(const vector<string>& relative_paths, bool skip_cache)
	{
		const uint32	  size		  = static_cast<uint32>(relative_paths.size());
		const string&	  working_dir = engine_data::get().get_working_dir();
		vector<meta*>	  resolved_metas(relative_paths.size());
		vector<void*>	  resolved_loaders(relative_paths.size());
		vector<string_id> resolved_types(relative_paths.size());

		for (uint32 i = 0; i < size; i++)
		{
			const string& path = relative_paths[i];
			const size_t  dot  = path.find_last_of(".");

			if (dot == string::npos)
				continue;

			const string	ext	 = path.substr(dot + 1, path.size() - dot - 1);
			resource_type	type = resource_type::resource_type_max;
			meta*			m	 = reflection::get().find_by_tag(ext.c_str());
			const string_id sid	 = TO_SID(path);

			const resource_storage& stg = _storages.at(m->get_type_index());
			if (stg.by_hashes.find(sid) != stg.by_hashes.end())
				continue;

			resolved_metas[i] = m;
			resolved_types[i] = m->get_type_id();
		}

		// Create loaders & load.
		vector<int> indices(relative_paths.size());
		std::iota(indices.begin(), indices.end(), 0);
		std::for_each(std::execution::par, indices.begin(), indices.end(), [&](int& i) {
			const string&	path			= relative_paths.at(i);
			const string_id sid				= TO_SID(path);
			meta*			reflection_meta = resolved_metas[i];

			if (reflection_meta == nullptr)
				return;

			const string full_path	= working_dir + path;
			const string cache_path = engine_data::get().get_cache_dir() + std::to_string(TO_SID(path)) + ".stkcache";

			void* loader = nullptr;

			if (!skip_cache && file_system::exists(cache_path.c_str()))
			{
				// Cook from the existing cache.
				istream stream = serialization::load_from_file(cache_path.c_str());
				loader		   = reflection_meta->invoke_function<void*, istream&>("cook_from_stream"_hs, stream);
				stream.destroy();
			}
			else
			{
				loader = reflection_meta->invoke_function<void*, const char*, world&>("cook_from_file"_hs, full_path.c_str(), _world);

				// Save to cache.
				if (loader)
				{
					ostream out_stream;
					reflection_meta->invoke_function<void, void*, ostream&>("serialize"_hs, loader, out_stream);
					serialization::save_to_file(cache_path.c_str(), out_stream);
					out_stream.destroy();
				}
			}

			resolved_loaders[i] = loader;
		});

		// Create the resources & assign.
		for (uint32 i = 0; i < size; i++)
		{
			meta* reflection_meta = resolved_metas[i];
			void* loader		  = resolved_loaders[i];
			if (loader == nullptr)
				continue;

			if (reflection_meta->has_function("create_from_raw"_hs))
			{
				vector<string> dependencies;
				if (reflection_meta->has_function("get_dependencies"_hs))
					reflection_meta->invoke_function<void, void*, vector<string>&>("get_dependencies"_hs, loader, dependencies);

				const resource_handle handle = reflection_meta->invoke_function<resource_handle, void*, world&>("create_from_raw"_hs, loader, _world);

				add_resource_watch(handle, relative_paths[i].c_str(), dependencies, resolved_types[i]);
			}
		}

		// Second pass.
		for (uint32 i = 0; i < size; i++)
		{
			meta* reflection_meta = resolved_metas[i];
			void* loader		  = resolved_loaders[i];
			if (loader == nullptr)
				continue;

			if (reflection_meta->has_function("create_from_raw_2"_hs))
			{
				vector<string> dependencies;
				if (reflection_meta->has_function("get_dependencies"_hs))
					reflection_meta->invoke_function<void, void*, vector<string>&>("get_dependencies"_hs, loader, dependencies);
				const resource_handle handle = reflection_meta->invoke_function<resource_handle, void*, world&>("create_from_raw_2"_hs, loader, _world);
				add_resource_watch(handle, relative_paths[i].c_str(), dependencies, resolved_types[i]);
			}
		}
	}

	void world_resources::add_resource_watch(resource_handle base_handle, const char* relative_path, const vector<string>& dependencies, string_id type)
	{
		_watched_resources.push_back({});
		resource_watch& w = _watched_resources.back();
		w.type_id		  = type;
		w.path			  = engine_data::get().get_working_dir() + relative_path;
		w.base_handle	  = base_handle;
		w.dependencies	  = dependencies;
		const uint16 id	  = static_cast<uint16>(_watched_resources.size() - 1);
		_file_watch.add_path(w.path.c_str(), id);

		for (const string& str : dependencies)
		{
			const string p = engine_data::get().get_working_dir() + str;
			_file_watch.add_path(p.c_str(), id);
		}
	}

	void world_resources::on_watched_resource_modified(const char* path, string_id last_modified, uint16 id)
	{
		SFG_ASSERT(id < _watched_resources.size());
		resource_watch& w = _watched_resources[id];

		meta& m = reflection::get().resolve(w.type_id);
		m.invoke_function<void, world&, resource_handle>("destroy"_hs, _world, w.base_handle);
		void* loader = m.invoke_function<void*, const char*, world&>("cook_from_file"_hs, w.path.c_str(), _world);

		const string cache_path = engine_data::get().get_cache_dir() + std::to_string(TO_SID(w.path)) + ".stkcache";

		ostream out_stream;
		m.invoke_function<void, void*, ostream&>("serialize"_hs, loader, out_stream);
		serialization::save_to_file(cache_path.c_str(), out_stream);
		out_stream.destroy();

		if (m.has_function("create_from_raw"_hs))
			w.base_handle = m.invoke_function<resource_handle, void*, world&>("create_from_raw"_hs, loader, _world);

		if (m.has_function("create_from_raw_2"_hs))
			w.base_handle = m.invoke_function<resource_handle, void*, world&>("create_from_raw_2"_hs, loader, _world);
	}

#endif

	void world_resources::load_resources(istream& stream)
	{
		const size_t size = stream.get_size();

		vector<pair<string_id, void*>> loaders;

		while (!stream.is_eof())
		{
			resource_type res_type = {};
			string_id	  sid	   = 0;
			string_id	  type_id  = 0;
			stream >> res_type;
			stream >> sid;
			stream >> type_id;

			meta& reflection_meta = reflection::get().resolve(type_id);
			void* loader		  = reflection_meta.invoke_function<void*, istream&>("cook_from_stream"_hs, stream);

			if (reflection_meta.has_function("create_from_raw"_hs))
				reflection_meta.invoke_function<void, void*, world&>("create_from_raw"_hs, loader, _world);
			loaders.push_back({type_id, loader});
		}

		// Second pass.
		for (auto [type_id, loader] : loaders)
		{
			meta& reflection_meta = reflection::get().resolve(type_id);
			if (reflection_meta.has_function("create_from_raw_2"_hs))
				reflection_meta.invoke_function<void, void*, world&>("create_from_raw_2"_hs, loader, _world);
		}
	}

}