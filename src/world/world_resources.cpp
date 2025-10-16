// Copyright (c) 2025 Inan Evin

#include "world_resources.hpp"
#include "gfx/world/world_renderer.hpp"
#include "gfx/renderer.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "app/debug_console.hpp"
#include "project/engine_data.hpp"
#include "world/world.hpp"
#include "world/common_world.hpp"
#include "world/traits/trait_model_instance.hpp"
#include "data/istream.hpp"
#include "data/ostream.hpp"
#include "reflection/reflection.hpp"
#include "data/pair.hpp"
#include "resources/texture_raw.hpp"
#include "resources/texture.hpp"
#include "resources/shader.hpp"
#include "resources/model.hpp"
#include "resources/mesh.hpp"

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

			if (skip_cache || !load_from_cache(reflection_meta, loader, path.c_str()))
			{
				loader = reflection_meta->invoke_function<void*, const char*, world&>("cook_from_file"_hs, full_path.c_str(), _world);

				if (loader)
					save_to_cache(reflection_meta, loader, path.c_str());
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
		resource_watch& w	   = _watched_resources.back();
		w.type_id			   = type;
		w.path				   = relative_path;
		w.base_handle		   = base_handle;
		w.dependencies		   = dependencies;
		const uint16 id		   = static_cast<uint16>(_watched_resources.size() - 1);
		const string full_path = engine_data::get().get_working_dir() + w.path;
		_file_watch.add_path(full_path.c_str(), id);

		for (const string& str : dependencies)
		{
			const string p = engine_data::get().get_working_dir() + str;
			_file_watch.add_path(p.c_str(), id);
		}
	}

	void world_resources::on_watched_resource_modified(const char* path, uint64 last_modified, uint16 id)
	{
		SFG_ASSERT(id < _watched_resources.size());
		resource_watch& w = _watched_resources[id];

		const resource_handle	prev_handle = w.base_handle;
		vector<resource_handle> prev_sub_handles;
		vector<string_id>		prev_sub_ids;

		if (w.type_id == type_id<model>::value)
		{
			model&				 m		= get_resource<model>(w.base_handle);
			const chunk_handle32 meshes = m.get_created_meshes();
			const uint16		 count	= m.get_mesh_count();
			if (count > 0)
			{
				resource_handle* mesh_handles = _aux_memory.get<resource_handle>(meshes);
				for (uint16 i = 0; i < count; i++)
				{
					const resource_handle handle = mesh_handles[i];
					prev_sub_handles.push_back(handle);
					mesh& mm = get_resource<mesh>(handle);
					prev_sub_ids.push_back(mm.get_sid());
				}
			}
		}

		meta& m = reflection::get().resolve(w.type_id);
		m.invoke_function<void, world&, resource_handle>("destroy"_hs, _world, w.base_handle);
		const string full_path = engine_data::get().get_working_dir() + w.path;
		void*		 loader	   = m.invoke_function<void*, const char*, world&>("cook_from_file"_hs, full_path.c_str(), _world);

		if (loader == nullptr)
			return;

		save_to_cache(&m, loader, w.path.c_str());

		if (m.has_function("create_from_raw"_hs))
			w.base_handle = m.invoke_function<resource_handle, void*, world&>("create_from_raw"_hs, loader, _world);

		if (m.has_function("create_from_raw_2"_hs))
			w.base_handle = m.invoke_function<resource_handle, void*, world&>("create_from_raw_2"_hs, loader, _world);

		const resource_handle new_handle = w.base_handle;
		if (w.type_id == type_id<shader>::value)
		{
			const render_event_resource_reloaded ev = {
				.prev_id = prev_handle.index,
				.new_id	 = new_handle.index,
			};

			_world.get_render_stream().add_event({.event_type = render_event_type::render_event_reload_shader}, ev);
		}
		else if (w.type_id == type_id<texture>::value)
		{
			const render_event_resource_reloaded ev = {
				.prev_id = prev_handle.index,
				.new_id	 = new_handle.index,
			};

			_world.get_render_stream().add_event({.event_type = render_event_type::render_event_reload_texture}, ev);
		}
		else if (w.type_id == type_id<model>::value)
		{
			vector<resource_handle> new_sub_handles;
			vector<string_id>		new_sids;

			model&				 m		= get_resource<model>(w.base_handle);
			const chunk_handle32 meshes = m.get_created_meshes();

			const uint16 count = m.get_mesh_count();
			if (count > 0)
			{
				resource_handle* mesh_handles = _aux_memory.get<resource_handle>(meshes);
				for (uint16 i = 0; i < count; i++)
				{
					const resource_handle handle = mesh_handles[i];
					const mesh&			  mm	 = get_resource<mesh>(handle);
					new_sub_handles.push_back(handle);
					new_sids.push_back(mm.get_sid());
				}
			}

			entity_manager& em				= _world.get_entity_manager();
			auto&			model_instances = em.get_trait_storage<trait_model_instance>();

			for (world_handle handle : model_instances)
			{
				trait_model_instance& mi = em.get_trait<trait_model_instance>(handle);
				if (mi.get_model() != prev_handle)
					continue;

				mi.instantiate_model_to_world(_world, w.base_handle);
			}
		}
	}

	void world_resources::save_to_cache(meta* reflection_meta, void* loader, const char* relative_path)
	{
		vector<string> dependencies;
		if (reflection_meta->has_function("get_dependencies"_hs))
			reflection_meta->invoke_function<void, void*, vector<string>&>("get_dependencies"_hs, loader, dependencies);

		ostream out_stream;

		const string	base_path		   = engine_data::get().get_working_dir() + relative_path;
		const string_id base_last_modified = file_system::get_last_modified_ticks(base_path);
		out_stream << base_last_modified;
		out_stream << static_cast<uint32>(dependencies.size());

		for (const string& dep : dependencies)
		{
			const string	p		 = engine_data::get().get_working_dir() + dep;
			const string_id modified = file_system::get_last_modified_ticks(p);
			out_stream << modified;
			out_stream << dep;
		}

		reflection_meta->invoke_function<void, void*, ostream&>("serialize"_hs, loader, out_stream);
		const string cache_path = engine_data::get().get_cache_dir() + std::to_string(TO_SID(relative_path)) + ".stkcache";
		serialization::save_to_file(cache_path.c_str(), out_stream);
		out_stream.destroy();
	}

	bool world_resources::load_from_cache(meta* reflection_meta, void*& loader, const char* relative_path)
	{
		const string cache_path = engine_data::get().get_cache_dir() + std::to_string(TO_SID(relative_path)) + ".stkcache";

		if (!file_system::exists(cache_path.c_str()))
			return false;

		istream stream = serialization::load_from_file(cache_path.c_str());

		const string base_path = engine_data::get().get_working_dir() + relative_path;

		if (!file_system::exists(base_path.c_str()))
		{
			stream.destroy();
			return false;
		}

		const string_id base_last_modified = file_system::get_last_modified_ticks(base_path);

		string_id stream_base_last_modified = 0;
		stream >> stream_base_last_modified;

		if (stream_base_last_modified != base_last_modified)
		{
			stream.destroy();
			return false;
		}

		uint32 dep_size = 0;
		stream >> dep_size;

		for (uint32 i = 0; i < dep_size; i++)
		{
			string_id dep_modified = 0;
			string	  dep_relative = "";
			stream >> dep_modified;
			stream >> dep_relative;

			const string p = engine_data::get().get_working_dir() + dep_relative;
			if (!file_system::exists(p.c_str()))
				continue;

			const string_id modified = file_system::get_last_modified_ticks(p);
			if (modified != dep_modified)
			{
				stream.destroy();
				return false;
			}
		}

		loader = reflection_meta->invoke_function<void*, istream&>("cook_from_stream"_hs, stream);
		stream.destroy();
		return true;
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