/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
