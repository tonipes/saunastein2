// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "render_proxy_resources.hpp"
#include "memory/pool_allocator_simple.hpp"

namespace SFG
{

	class buffer_queue;
	class texture_queue;
	class render_event_stream;

	class proxy_manager
	{
	public:
		proxy_manager(buffer_queue& b, texture_queue& t) : _buffer_queue(b), _texture_queue(t){};

		void init();
		void uninit();
		void fetch_render_events(render_event_stream& stream);

		inline render_proxy_texture& get_texture(uint16 idx)
		{
			return _textures.get(idx);
		}

		inline render_proxy_sampler& get_sampler(uint16 idx)
		{
			return _samplers.get(idx);
		}

		inline render_proxy_font& get_font(uint16 idx)
		{
			return _fonts.get(idx);
		}

		inline render_proxy_mesh& get_mesh(uint16 idx)
		{
			return _meshes.get(idx);
		}

		inline render_proxy_shader& get_shader(uint16 idx)
		{
			return _shaders.get(idx);
		}

		inline render_proxy_material& get_material(uint16 idx)
		{
			return _materials.get(idx);
		}

	private:
		void destroy_texture(render_proxy_texture& txt);

	private:
		buffer_queue&  _buffer_queue;
		texture_queue& _texture_queue;

		pool_allocator_simple<render_proxy_texture>	 _textures;
		pool_allocator_simple<render_proxy_sampler>	 _samplers;
		pool_allocator_simple<render_proxy_material> _materials;
		pool_allocator_simple<render_proxy_shader>	 _shaders;
		pool_allocator_simple<render_proxy_font>	 _fonts;
		pool_allocator_simple<render_proxy_mesh>	 _meshes;
	};
}