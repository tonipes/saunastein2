// Copyright (c) 2025 Inan Evin
#pragma once

#include "view.hpp"
#include "gpu_bone.hpp"
#include "gpu_light.hpp"
#include "gpu_entity.hpp"
#include "renderable.hpp"
#include "world/world_max_defines.hpp"
#include "data/static_vector.hpp"
#include "data/hash_map.hpp"

namespace SFG
{
	struct render_proxy_entity;
	struct render_proxy_camera;
	struct vector2ui16;
	class proxy_manager;

	class renderable_collector
	{
	public:
		// ------ lifecycle ------
		// -----------------------
		void reset();

		// ------ view gen ------
		// -----------------------
		void generate_view_from_cam(const render_proxy_entity& entity, const render_proxy_camera& cam, const vector2ui16& resolution);

		// ------ data ------
		// -----------------------
		void collect_entities(proxy_manager& pm);
		void collect_lights(proxy_manager& pm);
		void collect_model_instances(proxy_manager& pm);

		// ------ accessors ------
		// -----------------------
		inline const view& get_view() const
		{
			return _view;
		}

		inline const auto& get_entities() const
		{
			return _entities;
		}

		inline const auto& get_renderables() const
		{
			return _renderables;
		}

		inline const auto& get_point_lights() const
		{
			return _point_lights;
		}

		inline const auto& get_spot_lights() const
		{
			return _spot_lights;
		}

		inline const auto& get_dir_lights() const
		{
			return _dir_lights;
		}

		inline const auto& get_bones() const
		{
			return _bones;
		}

	private:
		view														   _view		= {};
		static_vector<renderable_object, MAX_WORLD_RENDERABLE_OBJECTS> _renderables = {};
		static_vector<gpu_entity, MAX_GPU_ENTITIES>					   _entities;
		static_vector<gpu_dir_light, MAX_WORLD_TRAIT_DIR_LIGHTS>	   _dir_lights;
		static_vector<gpu_point_light, MAX_WORLD_TRAIT_POINT_LIGHTS>   _point_lights;
		static_vector<gpu_spot_light, MAX_WORLD_TRAIT_SPOT_LIGHTS>	   _spot_lights;
		static_vector<gpu_bone, MAX_GPU_BONES>						   _bones;
		hash_map<uint32, uint32>									   _entity_index_to_buffer_index;
	};
}
