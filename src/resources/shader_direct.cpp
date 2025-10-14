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

	void shader_direct::create_from_raw(shader_raw& raw)
	{
		gfx_backend* backend = gfx_backend::get();
		_hw					 = backend->create_shader(raw.desc);
	}

	void shader_direct::destroy()
	{
		gfx_backend* backend = gfx_backend::get();
		backend->destroy_shader(_hw);
		_hw = NULL_GFX_ID;
	}

}
