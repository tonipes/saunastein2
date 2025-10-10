// Copyright (c) 2025 Inan Evin

#pragma once

#include "render_event_common.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{

	struct render_event_header
	{
		uint32			  index = 0;
		render_event_type event_type;
	};

	struct render_event
	{
		static constexpr size_t MAX_SIZE = 324;
		render_event_header		header;
		uint8					data[MAX_SIZE - sizeof(render_event_header)];

		template <typename T> T* construct() const
		{
			static_assert(sizeof(T) <= sizeof(data));
			return new (data) T();
		}

		template <typename T> void destruct()
		{
			reinterpret_cast<T*>(data)->~T();
		}
	};
}
