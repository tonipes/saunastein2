// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "common/string_id.hpp"
#include "common/type_id.hpp"
#include "data/string_view.hpp"

namespace SFG
{
	template <typename T> struct type_id;

	// clang-format off
#define REGISTER_TRAIT(T)           \
	template <> struct type_id<T>                              \
	{                                                          \
		static constexpr std::string_view name	= #T;          \
		static constexpr string_id		  value = fnv1a(name); \
	}
	// clang-format on

}
