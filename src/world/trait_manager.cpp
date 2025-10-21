// Copyright (c) 2025 Inan Evin

#include "trait_manager.hpp"
#include "world.hpp"

namespace SFG
{

	trait_manager::trait_manager(world& w) : _world(w)
	{
		_aux_memory.init(MAX_WORLD_TRAITS_AUX_MEMORY);
	}

	trait_manager::~trait_manager()
	{
		for (const trait_cache_storage& stg : _storages)
			delete stg.cache_ptr;

		_aux_memory.uninit();
	}

	void trait_manager::init()
	{
	}

	void trait_manager::uninit()
	{
		for (trait_cache_storage& stg : _storages)
			stg.cache_ptr->reset(_world);
		_aux_memory.reset();
	}

	world_handle trait_manager::add_trait(string_id type, world_handle entity)
	{
		trait_cache_storage& stg	= get_storage(type);
		const world_handle	 handle = stg.cache_ptr->add(entity, _world);
		_world.get_entity_manager().on_trait_added(entity, handle, type);
		return handle;
	}

	world_handle trait_manager::add_trait_from_stream(string_id type, istream& stream, world_handle entity)
	{
		trait_cache_storage& stg	= get_storage(type);
		const world_handle	 handle = stg.cache_ptr->add_from_stream(stream, entity, _world);
		_world.get_entity_manager().on_trait_added(entity, handle, type);
		return handle;
	}

	void trait_manager::save_trait_to_stream(string_id type, ostream& stream, world_handle trait)
	{
		trait_cache_storage& stg = get_storage(type);
		stg.cache_ptr->save_to_stream(stream, trait, _world);
	}

	void trait_manager::remove_trait(string_id type, world_handle entity, world_handle handle)
	{
		trait_cache_storage& stg = get_storage(type);
		_world.get_entity_manager().on_trait_removed(entity, handle, type);
		stg.cache_ptr->remove(handle, _world);
	}

	bool trait_manager::is_valid(string_id type, world_handle handle) const
	{
		const trait_cache_storage& stg = get_storage(type);
		return stg.cache_ptr->is_valid(handle);
	}

	void* trait_manager::get_trait(string_id type, world_handle handle) const
	{
		const trait_cache_storage& stg = get_storage(type);
		return stg.cache_ptr->get_ptr(handle);
	}

	const trait_manager::trait_cache_storage& trait_manager::get_storage(string_id type) const
	{
		auto it = std::find_if(_storages.begin(), _storages.end(), [type](const trait_cache_storage& stg) -> bool { return stg.type == type; });
		if (it == _storages.end())
		{
			SFG_ASSERT(false);
			throw std::runtime_error("Trait storage not found, did you register it?");
		}

		return *it;
	}

	trait_manager::trait_cache_storage& trait_manager::get_storage(string_id type)
	{
		auto it = std::find_if(_storages.begin(), _storages.end(), [type](const trait_cache_storage& stg) -> bool { return stg.type == type; });
		if (it == _storages.end())
		{
			SFG_ASSERT(false);
			throw std::runtime_error("Trait storage not found, did you register it?");
		}

		return *it;
	}

#ifdef SFG_TOOLMODE

	world_handle trait_manager::add_trait_from_json(string_id type, const json& j, world_handle entity)
	{
		const trait_cache_storage& stg = get_storage(type);
		return stg.cache_ptr->add_from_json(j, entity, _world);
	}

	void trait_manager::save_trait_to_json(string_id type, json& j, world_handle trait)
	{
		const trait_cache_storage& stg = get_storage(type);
		stg.cache_ptr->save_to_json(j, trait, _world);
	}

#endif
}