// Copyright (c) 2025 Inan Evin

#include "pool_allocator16.hpp"

namespace SFG
{

	pool_allocator16::~pool_allocator16()
	{
		if (_raw != nullptr)
			uninit();
	}

	void pool_allocator16::uninit()
	{
#ifdef ENABLE_MEMORY_TRACER
		memory_tracer::get().on_free(_raw);
#endif
		SFG_ASSERT(_raw != nullptr);
		reset();
		SFG_ALIGNED_FREE(_raw);
		_raw = nullptr;
	}

	void pool_allocator16::reset()
	{
		_head		= 0;
		_free_count = 0;

		uint16* free_indices = get_free_indices();
		uint8*	actives		 = get_actives();

		for (uint16 i = 0; i < _item_count; i++)
		{
			free_indices[i] = 0;
			actives[i]		= 0;
		}
	}

	bool pool_allocator16::is_valid(pool_handle16 handle) const
	{
		uint16* generations = get_generations();
		return handle.generation == generations[handle.index];
	}

}
