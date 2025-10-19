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
		proxy_manager(buffer_queue& b, texture_queue& t) : _buffer_queue(b), _texture_queue(t) {};

		void   init();
		void   uninit();
		void   fetch_render_events(render_event_stream& stream);
		void   flush_material_updates(uint8 frame_index);
		void   flush_destroys(bool force);
		gfx_id get_shader_variant(const render_proxy_material& mat, uint8 target_flags);

		inline render_proxy_texture& get_texture(resource_id idx)
		{
			return _textures.get(idx);
		}

		inline render_proxy_sampler& get_sampler(resource_id idx)
		{
			return _samplers.get(idx);
		}

		inline render_proxy_mesh& get_mesh(resource_id idx)
		{
			return _meshes.get(idx);
		}

		inline render_proxy_model& get_model(resource_id idx)
		{
			return _models.get(idx);
		}

		inline render_proxy_shader& get_shader(resource_id idx)
		{
			return _shaders.get(idx);
		}

		inline render_proxy_material& get_material(resource_id idx)
		{
			return _materials.get(idx);
		}

		inline render_proxy_entity& get_entity(world_id idx)
		{
			return _entities.get(idx);
		}

		inline render_proxy_mesh_instance& get_mesh_instance(world_id idx)
		{
			return _mesh_instances.get(idx);
		}

		inline render_proxy_camera& get_camera(world_id idx)
		{
			return _cameras.get(idx);
		}

		inline chunk_allocator32& get_aux()
		{
			return _aux_memory;
		}

		inline pool_allocator_simple<render_proxy_mesh_instance>& get_mesh_instances()
		{
			return _mesh_instances;
		}

		inline pool_allocator_simple<render_proxy_entity>& get_entities()
		{
			return _entities;
		}

		inline pool_allocator_simple<render_proxy_camera>& get_cameras()
		{
			return _cameras;
		}

		inline pool_allocator_simple<render_proxy_point_light>& get_point_lights()
		{
			return _point_lights;
		}

		inline pool_allocator_simple<render_proxy_spot_light>& get_spot_lights()
		{
			return _spot_lights;
		}

		inline pool_allocator_simple<render_proxy_dir_light>& get_dir_lights()
		{
			return _dir_lights;
		}

		inline render_proxy_ambient& get_ambient()
		{
			return _ambients.get(0);
		}

		inline world_id get_main_camera() const
		{
			return _main_camera_trait;
		}

		inline uint32 get_peak_mesh_instances() const
		{
			return _peak_mesh_instances;
		}

		inline uint32 get_peak_ambients() const
		{
			return _peak_mesh_instances;
		}

		inline uint32 get_peak_dir_lights() const
		{
			return _peak_mesh_instances;
		}

		inline uint32 get_peak_spot_lights() const
		{
			return _peak_spot_lights;
		}

		inline uint32 get_peak_point_lights() const
		{
			return _peak_point_lights;
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

		struct material_update
		{
			uint32 material_index = 0;
		};

		struct material_update_bucket
		{
			vector<material_update> updates;
		};

	private:
		void process_event(const render_event_header& header, istream& stream);
		void destroy_texture(render_proxy_texture& proxy);
		void destroy_sampler(render_proxy_sampler& proxy);
		void destroy_shader(render_proxy_shader& proxy);
		void destroy_material(render_proxy_material& proxy);
		void destroy_mesh(render_proxy_mesh& proxy);
		void destroy_model(render_proxy_model& proxy);
		void destroy_target_bucket(uint8 index);
		void add_to_destroy_bucket(const destroy_data& data, uint8 bucket);

	private:
		buffer_queue&  _buffer_queue;
		texture_queue& _texture_queue;

		chunk_allocator32								  _aux_memory;
		pool_allocator_simple<render_proxy_texture>		  _textures;
		pool_allocator_simple<render_proxy_sampler>		  _samplers;
		pool_allocator_simple<render_proxy_material>	  _materials;
		pool_allocator_simple<render_proxy_shader>		  _shaders;
		pool_allocator_simple<render_proxy_mesh>		  _meshes;
		pool_allocator_simple<render_proxy_model>		  _models;
		pool_allocator_simple<render_proxy_entity>		  _entities;
		pool_allocator_simple<render_proxy_mesh_instance> _mesh_instances;
		pool_allocator_simple<render_proxy_camera>		  _cameras;
		pool_allocator_simple<render_proxy_ambient>		  _ambients;
		pool_allocator_simple<render_proxy_dir_light>	  _dir_lights;
		pool_allocator_simple<render_proxy_spot_light>	  _spot_lights;
		pool_allocator_simple<render_proxy_point_light>	  _point_lights;
		world_id										  _main_camera_trait = 0;

		uint32 _peak_mesh_instances = 0;
		uint32 _peak_ambients		= 0;
		uint32 _peak_point_lights	= 0;
		uint32 _peak_spot_lights	= 0;
		uint32 _peak_dir_lights		= 0;

		static_vector<material_update_bucket, BACK_BUFFER_COUNT> _material_update_buckets;
		destroy_bucket											 _destroy_bucket[BACK_BUFFER_COUNT + 1];
	};
}