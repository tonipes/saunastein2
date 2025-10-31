// Copyright (c) 2025 Inan Evin

#include "resource_manager.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "data/pair.hpp"
#include "world/world_max_defines.hpp"

// resources
#include "resources/texture.hpp"
#include "resources/texture_raw.hpp"
#include "resources/texture_sampler.hpp"
#include "resources/texture_sampler_raw.hpp"

#ifdef SFG_TOOLMODE
#include "serialization/serialization.hpp"
#include "reflection/reflection.hpp"
#include "project/engine_data.hpp"
#include "resources/model.hpp"
#include "resources/mesh.hpp"
#include "resources/shader.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "world/traits/trait_model_instance.hpp"
#include "world/world.hpp"
#endif

#include <algorithm>
#include <execution>

namespace SFG
{
	resource_manager::resource_manager(world& w) : _world(w)
	{
		_aux_memory.init(MAX_WORLD_RESOURCES_AUX_MEMORY);
	}

	resource_manager::~resource_manager()
	{
		for (const cache_storage& stg : _storages)
			delete stg.cache_ptr;

		_aux_memory.uninit();
	}

	void resource_manager::init()
	{
		_file_watch.set_callback(std::bind(&resource_manager::on_watched_resource_modified, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
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

			if (dummy_color_data)
				SFG_MEMCPY(dummy_color_data, color_data, 4);

			if (dummy_normal_data)
				SFG_MEMCPY(dummy_normal_data, normal_data, 4);

			if (dummy_orm_data)
				SFG_MEMCPY(dummy_orm_data, orm_data, 4);

			dummy_color_raw.load_from_data(dummy_color_data, vector2ui16(1, 1), static_cast<uint8>(format::r8g8b8a8_srgb), false);
			dummy_normal_raw.load_from_data(dummy_normal_data, vector2ui16(1, 1), static_cast<uint8>(format::r8g8b8a8_unorm), false);
			dummy_orm_raw.load_from_data(dummy_orm_data, vector2ui16(1, 1), static_cast<uint8>(format::r8g8b8a8_unorm), false);

			_dummy_color_texture	  = add_resource<texture>(DUMMY_COLOR_TEXTURE_SID);
			_dummy_orm_texture		  = add_resource<texture>(DUMMY_ORM_TEXTURE_SID);
			_dummy_normal_texture	  = add_resource<texture>(DUMMY_NORMAL_TEXTURE_SID);
			texture& dummy_color_txt  = get_resource<texture>(_dummy_color_texture);
			texture& dummy_orm_txt	  = get_resource<texture>(_dummy_orm_texture);
			texture& dummy_normal_txt = get_resource<texture>(_dummy_normal_texture);
			dummy_color_txt.create_from_loader(dummy_color_raw, _world, _dummy_color_texture);
			dummy_normal_txt.create_from_loader(dummy_normal_raw, _world, _dummy_normal_texture);
			dummy_orm_txt.create_from_loader(dummy_orm_raw, _world, _dummy_orm_texture);
		}
	}

	void resource_manager::uninit()
	{
		for (cache_storage& stg : _storages)
		{
			stg.cache_ptr->reset(_world);
		}
		_aux_memory.reset();
		_file_watch.clear();
		_dynamic_sampler_count = 0;
	}

	void resource_manager::tick()
	{
#ifdef SFG_TOOLMODE
		_file_watch.tick();
#endif
	}

	resource_handle resource_manager::get_or_add_sampler(const sampler_desc& desc)
	{
		const cache_storage& storage = get_storage(type_id<texture_sampler>::value);

		auto& sampler_pool = underlying_pool<resource_cache<texture_sampler, texture_sampler_raw, MAX_WORLD_SAMPLERS>, texture_sampler>();

		resource_handle out_handle = {};

		auto it = sampler_pool.handles_begin();
		for (auto it = sampler_pool.handles_begin(); it != sampler_pool.handles_end(); ++it)
		{
			const resource_handle  h   = *it;
			const texture_sampler& smp = get_resource<texture_sampler>(h);
			if (smp.get_desc() == desc)
			{
				out_handle = h;
				break;
			}
		}
		if (out_handle.is_null())
		{
			out_handle			 = add_resource<texture_sampler>(_dynamic_sampler_count++);
			texture_sampler& smp = get_resource<texture_sampler>(out_handle);

			texture_sampler_raw raw = {};
			raw.desc				= desc;
			smp.create_from_loader(raw, _world, out_handle);
		}

		return out_handle;
	}

	void resource_manager::load_resources(istream& stream)
	{
		const size_t size = stream.get_size();

		vector<pair<string_id, void*>> loaders;
		const uint32				   max_passes = _max_load_priority + 1;

		while (!stream.is_eof())
		{
			string_id sid	  = 0;
			string_id type_id = 0;
			stream >> sid;
			stream >> type_id;

			resource_handle handle = {};

			void* loader = load_from_stream(type_id, stream);
			SFG_ASSERT(loader != nullptr);

			for (uint32 pass = 0; pass < max_passes; pass++)
			{
				handle = add_from_loader(type_id, loader, pass, sid);
				if (!handle.is_null())
					break;
			}

			SFG_ASSERT(!handle.is_null());
		}
	}

#ifdef SFG_TOOLMODE

	void resource_manager::load_resources(const vector<string>& relative_paths, bool skip_cache)
	{
		const uint32	  size		  = static_cast<uint32>(relative_paths.size());
		const string&	  working_dir = engine_data::get().get_working_dir();
		vector<void*>	  resolved_loaders(relative_paths.size());
		vector<string_id> resolved_types(relative_paths.size());

		vector<int> indices(relative_paths.size());
		std::iota(indices.begin(), indices.end(), 0);

		std::for_each(std::execution::par, indices.begin(), indices.end(), [&](int& i) {
			const string&	path = relative_paths.at(i);
			const string_id sid	 = TO_SID(path);

			const size_t dot = path.find_last_of(".");

			if (dot == string::npos)
			{
				SFG_ERR("Could not deduce extension: {0}", path);
				return;
			}

			const string ext = path.substr(dot + 1, path.size() - dot - 1);
			const meta*	 m	 = reflection::get().find_by_tag(ext.c_str());

			if (m == nullptr)
			{
				SFG_ASSERT(false, "No metadata found associated with this tag: {0}", ext);
				return;
			}

			const string	full_path = working_dir + path;
			const string_id type	  = m->get_type_id();
			void*			loader	  = nullptr;

			if (skip_cache || !load_from_cache(type, loader, path.c_str()))
			{
				loader = load_from_file(type, full_path.c_str());
				if (loader)
					save_to_cache(type, loader, path.c_str());
			}

			resolved_loaders[i] = loader;
			resolved_types[i]	= type;
		});

		// create actual resources.
		const uint32   max_passes = _max_load_priority + 1;
		vector<string> dependencies;

		for (uint32 pass = 0; pass < max_passes; pass++)
		{
			for (uint32 i = 0; i < size; i++)
			{
				const string&	p	   = relative_paths[i];
				const string_id type   = resolved_types[i];
				void*			loader = resolved_loaders[i];

				if (loader == nullptr)
					continue;

				const string_id hash = TO_SID(p);

				dependencies.resize(0);
				get_dependencies(type, loader, dependencies);

				resource_handle handle = {};
				handle				   = add_from_loader(type, loader, pass, hash);
				if (handle.is_null())
					continue;

				resolved_loaders[i] = nullptr;
				add_resource_watch(handle, p.c_str(), dependencies, type);
			}
		}
	}

	bool resource_manager::load_from_cache(string_id type, void*& loader, const char* releative_path)
	{
		const string cache_path = engine_data::get().get_cache_dir() + std::to_string(TO_SID(releative_path)) + ".stkcache";

		if (!file_system::exists(cache_path.c_str()))
			return false;

		istream stream = serialization::load_from_file(cache_path.c_str());

		const string base_path = engine_data::get().get_working_dir() + releative_path;

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

		loader = load_from_stream(type, stream);
		stream.destroy();
		return true;
	}

	void resource_manager::save_to_cache(string_id type, const void* loader, const char* relative_path)
	{
		vector<string> dependencies;
		get_dependencies(type, loader, dependencies);

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

		save_to_stream(type, loader, out_stream);
		const string cache_path = engine_data::get().get_cache_dir() + std::to_string(TO_SID(relative_path)) + ".stkcache";
		serialization::save_to_file(cache_path.c_str(), out_stream);
		out_stream.destroy();
	}

	void resource_manager::add_resource_watch(resource_handle base_handle, const char* relative_path, const vector<string>& dependencies, string_id type)
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

	void resource_manager::on_watched_resource_modified(const char* path, uint64 last_modified, uint16 id)
	{
		SFG_ASSERT(id < _watched_resources.size());
		resource_watch& w = _watched_resources[id];

		// load new resource
		const string full_path = engine_data::get().get_working_dir() + w.path;
		void*		 loader	   = load_from_file(w.type_id, full_path.c_str());
		if (loader == nullptr)
			return;

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

		destroy(w.type_id, w.base_handle);
		remove_resource(w.type_id, w.base_handle);

		save_to_cache(w.type_id, loader, w.path.c_str());

		const uint32 max_passes = _max_load_priority + 1;

		resource_handle new_handle = {};
		for (uint32 pass = 0; pass < max_passes; pass++)
		{
			new_handle = add_from_loader(w.type_id, loader, pass, TO_SID(w.path));
			if (!new_handle.is_null())
				break;
		}

		SFG_ASSERT(!new_handle.is_null());
		w.base_handle = new_handle;

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
		else if (w.type_id == type_id<texture_sampler>::value)
		{
			SFG_ASSERT(false);
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

			trait_manager& tm			   = _world.get_trait_manager();
			auto&		   model_instances = tm.underlying_pool<trait_cache<trait_model_instance, MAX_WORLD_MODEL_INSTANCES>, trait_model_instance>();
			for (trait_model_instance& mi : model_instances)
			{
				if (mi.get_model() != prev_handle)
					continue;

				mi.instantiate_model_to_world(_world, w.base_handle);
			}
		}

		SFG_INFO("Reloaded resource: {0}", path);
	}
#endif

	resource_handle resource_manager::add_resource(string_id type, string_id hash)
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->add(hash);
	}

	void resource_manager::remove_resource(string_id type, resource_handle handle)
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->remove(handle);
	}

	void* resource_manager::get_resource(string_id type, resource_handle handle) const
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->get_ptr(handle);
	}

	void* resource_manager::get_resource_by_hash(string_id type, string_id hash) const
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->get_by_hash_ptr(hash);
	}

	resource_handle resource_manager::get_resource_handle_by_hash(string_id type, string_id hash) const
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->get_handle_by_hash(hash);
	}

	string_id resource_manager::get_resource_hash(string_id type, resource_handle handle) const
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->get_hash(handle);
	}

	void* resource_manager::load_from_file(string_id type, const char* path) const
	{
		const cache_storage& stg	= get_storage(type);
		void*				 loader = stg.cache_ptr->load_from_file(path);
		return loader;
	}

	void* resource_manager::load_from_stream(string_id type, istream& stream) const
	{
		const cache_storage& stg	= get_storage(type);
		void*				 loader = stg.cache_ptr->load_from_stream(stream);
		return loader;
	}

	void resource_manager::save_to_stream(string_id type, const void* loader, ostream& stream) const
	{
		const cache_storage& stg = get_storage(type);
		stg.cache_ptr->save_to_stream(loader, stream);
	}

	void resource_manager::get_dependencies(string_id type, const void* loader, vector<string>& out_dependencies) const
	{
		const cache_storage& stg = get_storage(type);
		stg.cache_ptr->get_dependencies(loader, out_dependencies);
	}

	resource_handle resource_manager::add_from_loader(string_id type, void* loader, uint32 priority, string_id hash) const
	{
		const cache_storage& stg = get_storage(type);
		if (priority != stg.load_priority)
			return {};
		return stg.cache_ptr->add_from_loader(loader, _world, hash);
	}

	void resource_manager::destroy(string_id type, resource_handle handle)
	{
		const cache_storage& stg = get_storage(type);
		stg.cache_ptr->destroy(handle, _world);
	}

	bool resource_manager::is_valid(string_id type, resource_handle handle) const
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->is_valid(handle);
	}

	const resource_manager::cache_storage& resource_manager::get_storage(string_id type) const
	{
		auto it = std::find_if(_storages.begin(), _storages.end(), [type](const cache_storage& stg) -> bool { return stg.type == type; });
		if (it == _storages.end())
		{
			SFG_ASSERT(false);
			throw std::runtime_error("Storage can't be found, forgot to register it?");
		}
		return *it;
	}

	resource_manager::cache_storage& resource_manager::get_storage(string_id type)
	{
		auto it = std::find_if(_storages.begin(), _storages.end(), [type](const cache_storage& stg) -> bool { return stg.type == type; });
		if (it == _storages.end())
		{
			SFG_ASSERT(false);
			throw std::runtime_error("Storage can't be found, forgot to register it?");
		}
		return *it;
	}

}