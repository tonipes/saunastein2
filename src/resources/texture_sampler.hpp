// Copyright (c) 2025 Inan Evin

#pragma once

#include "resources/common_resources.hpp"
#include "reflection/resource_reflection.hpp"
#include "gfx/common/descriptions.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace SFG
{
	struct texture_sampler_raw;
	class world;

	class texture_sampler
	{
	public:
		void create_from_loader(const texture_sampler_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

		inline const sampler_desc& get_desc() const
		{
			return _desc;
		}

	private:
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif

		sampler_desc _desc = {};
	};

	REGISTER_RESOURCE(texture_sampler, "stksampler");

}
