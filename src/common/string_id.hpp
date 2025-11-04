// Copyright (c) 2025 Inan Evin

#pragma once

#include "common/size_definitions.hpp"
#include "data/string_view.hpp"

// Headers here.
namespace SFG
{
	using string_id = uint64;
	#define NULL_STRING_ID UINT64_MAX

	constexpr string_id hash_str(std::string_view str)
	{
		string_id h = 1469598103934665603ull;
		for (char c : str)
			h = (h ^ static_cast<unsigned char>(c)) * 1099511628211ull;
		return h;
	}

	consteval string_id fnv1a(string_view str)
	{
		string_id hash = 1469598103934665603ull;
		for (char c : str)
			hash = (hash ^ (string_id)c) * 1099511628211ull;
		return hash;
	}

	constexpr string_id operator"" _hs(const char* str, std::size_t len) noexcept
	{
		return hash_str({str, len});
	}

#define TO_SID(X)  hash_str(std::string_view{X}) // runtime
#define TO_SIDC(X) (X##_hs)						 // compile-time

}
