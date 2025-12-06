// Copyright (c) 2025 Inan Evin
#pragma once

#include "world/components/common_comps.hpp"
#include "reflection/component_reflection.hpp"
#include "resources/common_resources.hpp"
#include "memory/chunk_handle.hpp"
#include "data/vector.hpp"

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{
	class ostream;
	class istream;
	class world;
	class resource_manager;

	class comp_mesh_instance
	{
	public:
		// -----------------------------------------------------------------------------
		// trait
		// -----------------------------------------------------------------------------

		void on_add(world& we);
		void on_remove(world& w);
		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(const nlohmann::json& j, world& w);
#endif

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		void set_mesh(world& w, resource_handle model, resource_handle mesh, resource_handle skin, world_handle* skin_node_entities, uint16 skin_node_entity_count);

		inline resource_handle get_model() const
		{
			return _target_model;
		}

		inline resource_handle get_mesh() const
		{
			return _target_mesh;
		}

		inline const component_header& get_header() const
		{
			return _header;
		}

		inline chunk_handle32 get_skin_entities() const
		{
			return _skin_entities;
		}

		inline uint16 get_skin_entities_count() const
		{
			return _skin_entities_count;
		}

	private:
		template <typename T, int> friend class comp_cache;

		void fetch_refs(resource_manager& res, string_id& out_target, string_id& out_target_mesh) const;
		void fill_refs(resource_manager& res, string_id target, string_id target_mesh);

	private:
		component_header _header			  = {};
		resource_handle	 _target_model		  = {};
		resource_handle	 _target_mesh		  = {};
		resource_handle	 _target_skin		  = {};
		chunk_handle32	 _skin_entities		  = {};
		uint16			 _skin_entities_count = 0;
	};

	REGISTER_TRAIT(comp_mesh_instance);
}
