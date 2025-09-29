// Copyright (c) 2025 Inan Evin

#include "reflection/reflection.hpp"

namespace SFG
{
	meta* reflection::find_by_tag(const char* tag)
	{
		for (auto& [sid, m] : _metas)
		{
			if (strcmp(m.get_tag().c_str(), tag) == 0)
			{
				return &m;
			}
		}

		return nullptr;
	}
}
