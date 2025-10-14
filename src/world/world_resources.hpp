// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/type_id.hpp"
#include "data/hash_map.hpp"
#include "data/static_vector.hpp"
#include "memory/pool_allocator.hpp"
#include "memory/chunk_allocator.hpp"
#include "common/string_id.hpp"
#include "common_world.hpp"
#include "resources/common_resources.hpp"

#ifdef SFG_TOOLMODE
#include "io/simple_file_watcher.hpp"
#include "data/string.hpp"
#endif

namespace SFG
{
	class world;

	struct resource_storage
	{
		pool_allocator16					 storage;
		hash_map<string_id, resource_handle> by_hashes;
		string_id							 type_id = 0;
	};

	class istream;
	class texture;
	class material;
	class texture_sampler;
	class mesh;
	class render_event_stream;

	class world_resources
	{
	public:
#ifdef SFG_TOOLMODE
		struct resource_watch
		{
			string			path		 = "";
			string_id		type_id		 = 0;
			resource_handle base_handle	 = {};
			vector<string>	dependencies = {};
		};

#endif

	public:
		world_resources() = delete;
		world_resources(world& w);
		~world_resources();

		void init();
		void uninit();
		void tick();

#ifdef SFG_TOOLMODE
		void load_resources(const vector<string>& relative_paths, bool skip_cache = false);
		void add_resource_watch(resource_handle base_handle, const char* relative_path, const vector<string>& dependencies, string_id type);
		void on_watched_resource_modified(const char* path, uint64 last_modified, uint16 id);
#endif

		void load_resources(istream& in);

		template <typename T> void remove_resource(resource_handle handle)
		{
			const uint16 idx = type_id<T>::index;

			SFG_ASSERT(idx < _storages.size());
			resource_storage& stg = _storages[idx];
			stg.storage.free<T>(handle);

			auto it = stg.by_hashes.begin();
			for (; it != stg.by_hashes.end(); ++it)
			{
				if (it->second == handle)
				{
					stg.by_hashes.erase(it);
					break;
				}
			}
		}

		template <typename T> resource_handle get_resource_handle_by_hash(string_id hash) const
		{
			SFG_ASSERT(type_id<T>::index < _storages.size());
			resource_storage&	  stg	 = _storages[type_id<T>::index];
			const resource_handle handle = stg.by_hashes.at(hash);
			SFG_ASSERT(stg.storage.is_valid(handle));
			return handle;
		}

		template <typename T> resource_handle add_resource(string_id hash)
		{
			const uint16 idx = type_id<T>::index;
			SFG_ASSERT(idx < _storages.size());
			resource_storage&	  stg	 = _storages[idx];
			const resource_handle handle = stg.storage.allocate<T>();
			stg.by_hashes[hash]			 = handle;
			return handle;
		}

		template <typename T> T& get_resource(resource_handle handle) const
		{
			const uint16 idx = type_id<T>::index;
			SFG_ASSERT(idx < _storages.size());
			return _storages[type_id<T>::index].storage.get<T>(handle);
		}

		template <typename T> string_id get_resource_hash(resource_handle handle) const
		{
			const uint16 idx = type_id<T>::index;
			SFG_ASSERT(idx < _storages.size());
			resource_storage& stg = _storages[idx];
			SFG_ASSERT(stg.storage.is_valid(handle));

			for (auto& [sid, h] : stg.by_hashes)
			{
				if (h == handle)
					return sid;
			}

			return 0;
		}

		template <typename T> bool is_valid(resource_handle handle) const
		{
			const uint16 idx = type_id<T>::index;
			SFG_ASSERT(idx < _storages.size());
			return _storages[idx].storage.is_valid(handle);
		}

		template <typename T> bool is_valid(string_id hash) const
		{
			SFG_ASSERT(type_id<T>::index < _storages.size());
			resource_storage& stg = _storages[type_id<T>::index];
			return stg.by_hashes.find(hash) != stg.by_hashes.end();
		}

		template <typename T> T& get_resource_by_hash(string_id hash) const
		{
			SFG_ASSERT(type_id<T>::index < _storages.size());
			resource_storage&	  stg	 = _storages[type_id<T>::index];
			const resource_handle handle = stg.by_hashes.at(hash);
			return stg.storage.get<T>(handle);
		}

		template <typename T> void init_storage(uint32 count)
		{
			const uint16 idx = type_id<T>::index;

			if (_storages.size() <= idx)
				_storages.resize(idx + 1);

			resource_storage& stg = _storages[idx];
			stg.type_id			  = type_id<T>::value;
			stg.storage.init<T>(count);
		}

		inline static_vector<resource_storage, resource_type_max>& get_storages()
		{
			return _storages;
		}

		inline chunk_allocator32& get_aux()
		{
			return _aux_memory;
		}

	private:
		world& _world;

		mutable static_vector<resource_storage, resource_type_max> _storages;
		chunk_allocator32										   _aux_memory;
		resource_handle											   _dummy_color_texture	 = {};
		resource_handle											   _dummy_orm_texture	 = {};
		resource_handle											   _dummy_normal_texture = {};

#ifdef SFG_TOOLMODE
		simple_file_watcher	   _file_watch;
		vector<resource_watch> _watched_resources;
#endif
	};
}
