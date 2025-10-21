// Copyright (c) 2025 Inan Evin

#include "reflection/reflection.hpp"

namespace SFG
{
	const meta* reflection::find_by_tag(const char* tag) const
	{
		const string_id tagsid = TO_SID(tag);
		for (const auto& [sid, m] : _metas)
		{
			if (m.get_tag() == tagsid)
			{
				return &m;
			}
		}

		return nullptr;
	}
}
