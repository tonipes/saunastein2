// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "render_proxy_resources.hpp"
#include "render_proxy_entity.hpp"
#include "render_proxy_traits.hpp"
#include "memory/pool_allocator_simple.hpp"
#include "memory/chunk_allocator.hpp"
#include "data/vector.hpp"
#include "gfx/common/gfx_constants.hpp"

namespace SFG
{

	class buffer_queue;
	class texture_queue;
	class render_event_stream;
	class istream;
	struct render_event_header;

	class proxy_manager
	{
	public:
		proxy_manager(buffer_queue& b, texture_queue& t) : _buffer_queue(b), _texture_queue(t){};

		void init();
		void uninit();
		void fetch_render_events(render_event_stream& stream);
		void flush_destroys(bool force);

		inline render_proxy_texture& get_texture(uint32 idx)
		{
			return _textures.get(idx);
		}

		inline render_proxy_sampler& get_sampler(uint32 idx)
		{
			return _samplers.get(idx);
		}

		inline render_proxy_mesh& get_mesh(uint32 idx)
		{
			return _meshes.get(idx);
		}

		inline render_proxy_shader& get_shader(uint32 idx)
		{
			return _shaders.get(idx);
		}

		inline render_proxy_material& get_material(uint32 idx)
		{
			return _materials.get(idx);
		}

		inline render_proxy_entity& get_entity(uint32 idx)
		{
			return _entities.get(idx);
		}

		inline render_proxy_model_instance& get_model_instance(uint32 idx)
		{
			return _model_instances.get(idx);
		}

		inline chunk_allocator32& get_aux()
		{
			return _aux_memory;
		}

	private:
		enum destroy_data_type : uint8
		{
			texture,
			sampler,
			shader,
			bind_group,
			resource,
		};

		struct destroy_data
		{
			gfx_id			  id   = 0;
			destroy_data_type type = {};
		};

		struct destroy_bucket
		{
			vector<destroy_data> list;
		};

	private:
		void process_event(const render_event_header& header, istream& stream);
		void destroy_texture(render_proxy_texture& proxy);
		void destroy_sampler(render_proxy_sampler& proxy);
		void destroy_shader(render_proxy_shader& proxy);
		void destroy_material(render_proxy_material& proxy);
		void destroy_mesh(render_proxy_mesh& proxy);
		void destroy_target_bucket(uint8 index);
		void add_to_destroy_bucket(const destroy_data& data, uint8 bucket);

	private:
		buffer_queue&  _buffer_queue;
		texture_queue& _texture_queue;

		chunk_allocator32								   _aux_memory;
		pool_allocator_simple<render_proxy_texture>		   _textures;
		pool_allocator_simple<render_proxy_sampler>		   _samplers;
		pool_allocator_simple<render_proxy_material>	   _materials;
		pool_allocator_simple<render_proxy_shader>		   _shaders;
		pool_allocator_simple<render_proxy_mesh>		   _meshes;
		pool_allocator_simple<render_proxy_entity>		   _entities;
		pool_allocator_simple<render_proxy_model_instance> _model_instances;

		destroy_bucket _destroy_bucket[FRAMES_IN_FLIGHT + 1];
		uint8		   _destroy_bucket_index = 0;
	};
}