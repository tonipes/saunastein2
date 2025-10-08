// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"

namespace SFG
{
	template <typename T> struct type_id;

	// clang-format off
#define REGISTER_RESOURCE(T, INDEX, REFLECTION_CLASS)           \
	template <> struct type_id<T>                              \
	{                                                          \
		static constexpr std::string_view name	= #T;          \
		static constexpr string_id		  value = fnv1a(name); \
		static constexpr uint16			  index = INDEX;       \
		static inline REFLECTION_CLASS reflection_instance = {};       \
	}

	// clang-format on

}
