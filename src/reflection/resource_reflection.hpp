// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "common/string_id.hpp"
#include "reflection/reflection.hpp"

namespace SFG
{
	template <typename T> struct type_id;

	template <typename T> struct ref_register
	{
		ref_register(string_id type, const string& tag)
		{
			reflection::get().register_meta(type, 0, tag);
		}
	};
	// clang-format off
#define REGISTER_RESOURCE(T, TAG)           \
	template <> struct type_id<T>                              \
	{                                                          \
		static constexpr std::string_view name	= #T;          \
		static constexpr string_id		  value = fnv1a(name); \
		static inline ref_register<T>  reflection = ref_register<T>(value, TAG); \
	}
	// clang-format on
}
