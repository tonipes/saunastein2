// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/vector.hpp"

namespace SFG
{
	namespace
	{

		template <typename T> struct is_vector : std::false_type
		{
		};

		template <typename U> struct is_vector<std::vector<U>> : std::true_type
		{
		};

		template <typename U> struct is_vector<const std::vector<U>> : std::true_type
		{
		};

		template <typename T> inline constexpr bool is_vector_v = is_vector<T>::value;

	}
}
