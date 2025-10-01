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
#include "resources/texture.hpp"
#include "resources/texture_sampler.hpp"
#include "resources/material.hpp"
#include "resources/mesh.hpp"
#include "resources/shader.hpp"
#include "resources/font.hpp"

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
		_aux_memory.init(1024 * 1024 * 4);

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
			stg.storage.uninit();

		_aux_memory.uninit();
	}

	void world_resources::init()
	{
		_file_watch.set_callback(std::bind(&world_resources::on_watched_resource_modified, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		_modified_resources.reserve(250);
		_reuse_reload_resources.reserve(250);
		_file_watch.reserve(250);
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

		if (!_modified_resources.empty())
		{
			// TODO: We gotta stop render here?

			for (uint16 id : _modified_resources)
				_reuse_reload_resources.push_back(_watched_resources[id].base_path);
			load_resources(_reuse_reload_resources, true);
			_reuse_reload_resources.resize(0);
			_modified_resources.resize(0);
		}
#endif
	}

#ifdef SFG_TOOLMODE
	void world_resources::load_resources(const vector<string>& relative_paths, bool skip_cache)
	{
		const uint32  size		  = static_cast<uint32>(relative_paths.size());
		const string& working_dir = engine_data::get().get_working_dir();
		vector<meta*> resolved_metas(relative_paths.size());
		vector<void*> resolved_loaders(relative_paths.size());

		for (uint32 i = 0; i < size; i++)
		{
			const string& path = relative_paths[i];
			const size_t  dot  = path.find_last_of(".");

			if (dot == string::npos)
				continue;

			const string  ext  = path.substr(dot + 1, path.size() - dot - 1);
			resource_type type = resource_type::resource_type_allowed_max;
			resolved_metas[i]  = reflection::get().find_by_tag(ext.c_str());
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
			const string cache_path = engine_data::get().get_cache_dir() + path;

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
				loader = reflection_meta->invoke_function<void*, const char*>("cook_from_file"_hs, full_path.c_str());

				_file_watch.add_path(full_path.c_str());

				if (reflection_meta->has_function("get_dependencies"_hs))
				{
					vector<string> dependencies;
					reflection_meta->invoke_function<void, void*, vector<string>&>("get_dependencies"_hs, loader, dependencies);

					for (const string& str : dependencies)
					{
					}
				}

				// Save to cache.
				ostream out_stream;
				reflection_meta->invoke_function<void*, void*, ostream&>("serialize"_hs, loader, out_stream);
				serialization::save_to_file(cache_path.c_str(), out_stream);
				out_stream.destroy();
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
				const resource_handle handle = reflection_meta->invoke_function<resource_handle, void*, world&>("create_from_raw"_hs, loader, _world);
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
				const resource_handle handle = reflection_meta->invoke_function<resource_handle, void*, world&>("create_from_raw_2"_hs, loader, _world);
			}
		}
	}

	void world_resources::add_resource_watch(string_id type, const char* base_path, const vector<string>& dependency_paths)
	{
		for (const resource_watch& w : _watched_resources)
		{
			if (w.base_path.compare(base_path) == 0)
				return;
		}

		const uint16 id = static_cast<uint16>(_watched_resources.size());
		_watched_resources.push_back({base_path, type});

		_file_watch.add_path(base_path, id);

		for (const string& p : dependency_paths)
			_file_watch.add_path(p.c_str(), id);
	}

	void world_resources::on_watched_resource_modified(const char* path, string_id last_modified, uint16 id)
	{
		SFG_ASSERT(id < _watched_resources.size());
		resource_watch& w = _watched_resources[id];

		meta& m = reflection::get().resolve(w.type_id);
		m.invoke_function<void, world&>("destroy"_hs, _world);
		_modified_resources.push_back(id);
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