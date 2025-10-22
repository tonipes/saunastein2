// Copyright (c) 2025 Inan Evin

#include "shader_direct.hpp"
#include "shader_raw.hpp"
#include "gfx/backend/backend.hpp"
#include "io/assert.hpp"

namespace SFG
{
	shader_direct::~shader_direct()
	{
		SFG_ASSERT(_hw == NULL_GFX_ID);
	}

	void shader_direct::create_from_loader(shader_raw& raw, gfx_id layout)
	{
		gfx_backend* backend = gfx_backend::get();

		shader_desc desc = raw.pso_variants.at(0).desc;
		_hw				 = backend->create_shader(desc, raw.compile_variants.at(0).blobs, layout);
	}

	void shader_direct::destroy()
	{
		gfx_backend* backend = gfx_backend::get();
		backend->destroy_shader(_hw);
		_hw = NULL_GFX_ID;
	}

}
