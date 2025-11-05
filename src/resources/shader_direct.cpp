// Copyright (c) 2025 Inan Evin

#include "shader_direct.hpp"
#include "shader_raw.hpp"
#include "gfx/backend/backend.hpp"
#include "io/assert.hpp"

namespace SFG
{
	shader_direct::~shader_direct()
	{
		SFG_ASSERT(_hws.empty());
	}

	void shader_direct::create_from_loader(shader_raw& raw, gfx_id layout)
	{
		gfx_backend* backend = gfx_backend::get();

		for (const pso_variant& v : raw.pso_variants)
		{
			hw h	= {};
			h.flags = v.variant_flags;
			h.id	= backend->create_shader(v.desc, raw.compile_variants.at(v.compile_variant).blobs, layout);
			_hws.push_back(h);
		}
	}

	void shader_direct::destroy()
	{
		gfx_backend* backend = gfx_backend::get();

		for (const hw& h : _hws)
		{
			backend->destroy_shader(h.id);
		}
		_hws.clear();
	}

	uint16 shader_direct::get_hw(uint32 variant_flags) const
	{
		for (const hw& h : _hws)
		{
			if (h.flags.is_all_set(variant_flags))
				return h.id;
		}

		SFG_ASSERT(false);
		return 0;
	}

}
