// Copyright (c) 2025 Inan Evin

#pragma once

#include "size_definitions.hpp"
#include "string_id.hpp"

namespace SFG
{

	template <typename T> struct type_id;

#define REGISTER_TYPE(T, INDEX)                                                                                                                                                                                                                                    \
	template <> struct type_id<T>                                                                                                                                                                                                                                  \
	{                                                                                                                                                                                                                                                              \
		static constexpr std::string_view name	= #T;                                                                                                                                                                                                              \
		static constexpr string_id		  value = fnv1a(name);                                                                                                                                                                                                     \
		static constexpr uint16			  index = INDEX;                                                                                                                                                                                                           \
	}
}
