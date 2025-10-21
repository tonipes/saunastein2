// Copyright (c) 2025 Inan Evin
#pragma once

#include "world/traits/common_trait.hpp"
#include "reflection/trait_reflection.hpp"
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
	class render_event_stream;

	struct trait_mesh_instance_reflection
	{
		trait_mesh_instance_reflection();
	};

	class trait_mesh_instance
	{
	public:
		void set_mesh(world& w, resource_handle model, resource_handle mesh);

		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(nlohmann::json& j, world& w);
#endif

		inline resource_handle get_model() const
		{
			return _target_model;
		}

		inline resource_handle get_mesh() const
		{
			return _target_mesh;
		}

		inline const trait_header& get_header() const
		{
			return _header;
		}

	private:
		friend class entity_manager;
		friend struct trait_mesh_instance_reflection;

		void on_add(world& we);
		void on_remove(world& w);
		void fetch_refs(resource_manager& res, string_id& out_target, string_id& out_target_mesh) const;
		void fill_refs(resource_manager& res, string_id target, string_id target_mesh);

	private:
		trait_header	_header		  = {};
		resource_handle _target_model = {};
		resource_handle _target_mesh  = {};
	};

	REGISTER_TRAIT(trait_mesh_instance, trait_types::trait_type_mesh_instance, trait_mesh_instance_reflection);
}
