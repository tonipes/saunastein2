// Copyright (c) 2025 Inan Evin

#include "proxy_manager.hpp"
#include "resources/common_resources.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_event_storage_gfx.hpp"
#include "gfx/texture_queue.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/backend/backend.hpp"

namespace SFG
{

	void proxy_manager::init()
	{
		_shaders.init(MAX_WORLD_SHADERS);
		_textures.init(MAX_WORLD_TEXTURES);
		_samplers.init(MAX_WORLD_SAMPLERS);
		_materials.init(MAX_WORLD_MATERIALS);
		_meshes.init(MAX_WORLD_MESHES);
		_fonts.init(MAX_WORLD_FONTS);
	}

	void proxy_manager::uninit()
	{
		for (auto& res : _textures)
		{
			if (res.active == 0)
				continue;

			destroy_texture(res);
		}

		_shaders.uninit();
		_textures.uninit();
		_samplers.uninit();
		_materials.uninit();
		_meshes.uninit();
		_fonts.uninit();
	}

	void proxy_manager::fetch_render_events(render_event_stream& stream)
	{
		auto&		  events  = stream.get_events();
		render_event* ev	  = events.peek();
		gfx_backend*  backend = gfx_backend::get();

		while (ev != nullptr)
		{
			if (ev->header.event_type == render_event_type::render_event_create_texture)
			{
				render_event_storage_texture* stg = reinterpret_cast<render_event_storage_texture*>(ev->data);

				render_proxy_texture& proxy = get_texture(ev->header.handle.index);
				proxy.active				= 1;

				proxy.hw = backend->create_texture({
					.texture_format = static_cast<format>(stg->format),
					.size			= stg->size,
					.flags			= texture_flags::tf_is_2d | texture_flags::tf_sampled,
					.views			= {{}},
					.mip_levels		= static_cast<uint8>(stg->buffers.size()),
					.array_length	= 1,
					.samples		= 1,
					.debug_name		= stg->name,
				});

				SFG_FREE((void*)stg->name);

				proxy.intermediate = backend->create_resource({
					.size		= stg->intermediate_size,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "texture_intermediate",
				});

				_texture_queue.add_request(stg->buffers, proxy.hw, proxy.intermediate, 1);
			}
			else if (ev->header.event_type == render_event_type::render_event_destroy_texture)
			{
				render_proxy_texture& proxy = get_texture(ev->header.handle.index);
				destroy_texture(proxy);
			}

			events.pop();
			ev = events.peek();
		}
	}

	void proxy_manager::destroy_texture(render_proxy_texture& proxy)
	{
		gfx_backend* backend = gfx_backend::get();
		backend->destroy_texture(proxy.hw);
		backend->destroy_resource(proxy.intermediate);
		proxy.active = 0;
	}
}