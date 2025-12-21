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

#include "pool_handle.hpp"
#include "io/assert.hpp"
#include "memory.hpp"
#include "data/vector.hpp"
#include <new>

namespace SFG
{

	template <typename T, typename U, int N> struct static_pool_allocator
	{
		T		  data[N];
		vector<U> free_list = {};
		U		  head		= 0;

		inline U add()
		{
			if (!free_list.empty())
			{
				const U id = free_list.back();
				free_list.pop_back();
				return id;
			}

			const U id = head;
			head++;
			SFG_ASSERT(head < N);
			return id;
		}

		void remove(U id)
		{
			free_list.push_back(id);
			data[id] = T();
		}

		T& get(U id)
		{
			SFG_ASSERT(id < N);
			return data[id];
		}

		const T& get(U id) const
		{
			SFG_ASSERT(id < N);
			return data[id];
		}

		inline void verify_uninit()
		{
			SFG_ASSERT(static_cast<U>(free_list.size()) == head);
		}

		void reset()
		{
			for (int i = 0; i < N; i++)
				data[i] = {};

			free_list.resize(0);
			head = 0;
		}
	};

}
