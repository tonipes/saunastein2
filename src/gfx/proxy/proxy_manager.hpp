// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "render_proxy_resources.hpp"
#include "render_proxy_entity.hpp"
#include "render_proxy_components.hpp"
#include "memory/pool_allocator_simple.hpp"
#include "memory/chunk_allocator.hpp"
#include "data/vector.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "game/game_max_defines.hpp"

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
		proxy_manager() = delete;
		proxy_manager(buffer_queue& b, texture_queue& t);
		~proxy_manager();

		void   init();
		void   uninit();
		void   fetch_render_events(render_event_stream& stream, uint8 frame_index);
		void   flush_destroys(bool force);
		gfx_id get_shader_variant(resource_id shader_handle, uint32 flags);

		inline render_proxy_texture& get_texture(resource_id idx)
		{
			return _textures->get(idx);
		}

		inline render_proxy_sampler& get_sampler(resource_id idx)
		{
			return _samplers->get(idx);
		}

		inline render_proxy_mesh& get_mesh(resource_id idx)
		{
			return _meshes->get(idx);
		}

		inline render_proxy_skin& get_skin(resource_id idx)
		{
			return _skins->get(idx);
		}

		inline render_proxy_model& get_model(resource_id idx)
		{
			return _models->get(idx);
		}

		inline render_proxy_shader& get_shader(resource_id idx)
		{
			return _shaders->get(idx);
		}

		inline render_proxy_material& get_material(resource_id idx)
		{
			return _materials->get(idx);
		}

		inline render_proxy_material_runtime& get_material_runtime(resource_id idx)
		{
			return _material_runtimes->get(idx);
		}

		inline render_proxy_entity& get_entity(world_id idx)
		{
			return _entities->get(idx);
		}

		inline render_proxy_mesh_instance& get_mesh_instance(world_id idx)
		{
			return _mesh_instances->get(idx);
		}

		inline render_proxy_camera& get_camera(world_id idx)
		{
			return _cameras->get(idx);
		}

		inline render_proxy_ambient& get_ambient()
		{
			return _ambients->get(0);
		}

		inline render_proxy_canvas& get_canvas(world_id idx)
		{
			return _canvases->get(idx);
		}

		inline chunk_allocator32& get_aux()
		{
			return _aux_memory;
		}

		inline auto get_mesh_instances()
		{
			return _mesh_instances;
		}

		inline auto get_entities()
		{
			return _entities;
		}

		inline auto get_skins()
		{
			return _skins;
		}

		inline auto& get_cameras()
		{
			return _cameras;
		}

		inline auto get_point_lights()
		{
			return _point_lights;
		}

		inline auto get_spot_lights()
		{
			return _spot_lights;
		}

		inline auto get_dir_lights()
		{
			return _dir_lights;
		}

		inline auto get_materials()
		{
			return _materials;
		}

		inline auto get_material_runtimes()
		{
			return _material_runtimes;
		}

		inline auto get_canvases()
		{
			return _canvases;
		}

		inline world_id get_main_camera() const
		{
			return _main_camera_trait;
		}

		inline uint32 get_peak_mesh_instances() const
		{
			return _peak_mesh_instances + 1;
		}

		inline uint32 get_ambient_exists() const
		{
			return _ambient_exists;
		}

		inline uint32 get_peak_dir_lights() const
		{
			return _peak_dir_lights + 1;
		}

		inline uint32 get_peak_spot_lights() const
		{
			return _peak_spot_lights + 1;
		}

		inline uint32 get_peak_point_lights() const
		{
			return _peak_point_lights + 1;
		}

		inline uint32 get_count_dir_lights() const
		{
			return _count_dir_lights;
		}

		inline uint32 get_count_spot_lights() const
		{
			return _count_spot_lights;
		}

		inline uint32 get_count_point_lights() const
		{
			return _count_point_lights;
		}

		inline uint32 get_peak_entities() const
		{
			return _peak_entities + 1;
		}

		inline uint32 get_peak_canvases() const
		{
			return _peak_canvases + 1;
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
		void process_event(const render_event_header& header, istream& stream, uint8 frame_index);
		void destroy_texture(render_proxy_texture& proxy);
		void destroy_sampler(render_proxy_sampler& proxy);
		void destroy_shader(render_proxy_shader& proxy);
		void destroy_material(render_proxy_material& proxy);
		void destroy_mesh(render_proxy_mesh& proxy);
		void destroy_model(render_proxy_model& proxy);
		void destroy_canvas(render_proxy_canvas& proxy);
		void destroy_skin(render_proxy_skin& proxy);
		void destroy_target_bucket(uint8 index);
		void add_to_destroy_bucket(const destroy_data& data, uint8 bucket);
		void reset();

	private:
		buffer_queue&  _buffer_queue;
		texture_queue& _texture_queue;

		using textures_type			 = pool_allocator_simple<render_proxy_texture, MAX_WORLD_TEXTURES>;
		using samplers_type			 = pool_allocator_simple<render_proxy_sampler, MAX_WORLD_SAMPLERS>;
		using materials_type		 = pool_allocator_simple<render_proxy_material, MAX_WORLD_MATERIALS>;
		using material_runtimes_type = pool_allocator_simple<render_proxy_material_runtime, MAX_WORLD_MATERIALS>;
		using shaders_type			 = pool_allocator_simple<render_proxy_shader, MAX_WORLD_SHADERS>;
		using meshes_type			 = pool_allocator_simple<render_proxy_mesh, MAX_WORLD_MESHES>;
		using skins_type			 = pool_allocator_simple<render_proxy_skin, MAX_WORLD_SKINS>;
		using models_type			 = pool_allocator_simple<render_proxy_model, MAX_WORLD_MODELS>;
		using entity_type			 = pool_allocator_simple<render_proxy_entity, MAX_ENTITIES>;
		using mesh_instances_type	 = pool_allocator_simple<render_proxy_mesh_instance, MAX_WORLD_COMP_MESH_INSTANCES>;
		using cameras_type			 = pool_allocator_simple<render_proxy_camera, MAX_WORLD_COMP_CAMERAS>;
		using ambients_type			 = pool_allocator_simple<render_proxy_ambient, MAX_WORLD_COMP_AMBIENT>;
		using point_lights_type		 = pool_allocator_simple<render_proxy_point_light, MAX_WORLD_COMP_POINT_LIGHTS>;
		using spot_lights_type		 = pool_allocator_simple<render_proxy_spot_light, MAX_WORLD_COMP_SPOT_LIGHTS>;
		using dir_lights_type		 = pool_allocator_simple<render_proxy_dir_light, MAX_WORLD_COMP_DIR_LIGHTS>;
		using canvas_type			 = pool_allocator_simple<render_proxy_canvas, MAX_WORLD_COMP_CANVAS>;

		destroy_bucket _destroy_bucket[BACK_BUFFER_COUNT + 1];

		chunk_allocator32		_aux_memory;
		textures_type*			_textures		   = nullptr;
		samplers_type*			_samplers		   = nullptr;
		materials_type*			_materials		   = nullptr;
		material_runtimes_type* _material_runtimes = nullptr;
		shaders_type*			_shaders		   = nullptr;
		meshes_type*			_meshes			   = nullptr;
		skins_type*				_skins			   = nullptr;
		models_type*			_models			   = nullptr;
		entity_type*			_entities		   = nullptr;
		mesh_instances_type*	_mesh_instances	   = nullptr;
		cameras_type*			_cameras		   = nullptr;
		ambients_type*			_ambients		   = nullptr;
		point_lights_type*		_point_lights	   = nullptr;
		spot_lights_type*		_spot_lights	   = nullptr;
		dir_lights_type*		_dir_lights		   = nullptr;
		canvas_type*			_canvases		   = nullptr;

		world_id _main_camera_trait	  = NULL_WORLD_ID;
		uint32	 _peak_mesh_instances = 0;
		uint32	 _peak_point_lights	  = 0;
		uint32	 _peak_spot_lights	  = 0;
		uint32	 _peak_dir_lights	  = 0;
		uint32	 _count_dir_lights	  = 0;
		uint32	 _count_spot_lights	  = 0;
		uint32	 _count_point_lights  = 0;
		uint32	 _peak_entities		  = 0;
		uint32	 _peak_canvases		  = 0;
		uint8	 _ambient_exists	  = 0;
	};
}