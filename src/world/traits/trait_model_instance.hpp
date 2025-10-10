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
	class world_resources;
	class render_event_stream;

	struct trait_model_instance_reflection
	{
		trait_model_instance_reflection();
	};

	class trait_model_instance
	{
	public:
		void set_model(resource_handle handle, world& w, render_event_stream& stream, resource_handle target_mesh = {});
		void set_model(string_id sid, world& w, render_event_stream& stream, resource_handle target_mesh = {});

		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(nlohmann::json& j, world& w);
#endif

	private:
		friend class entity_manager;

		void on_add(world& w, trait_handle handle, entity_handle entity);
		void on_remove(world& w, trait_handle handle);
		void fetch_refs(world_resources& res, string_id& out_target, string_id& out_target_mesh, vector<string_id>& out_materials) const;
		void fill_refs(world_resources& res, string_id target, string_id target_mesh, const vector<string_id>& materials);

	private:
		trait_header	_header			= {};
		resource_handle _target_model	= {};
		resource_handle _target_mesh	= {};
		chunk_handle32	_materials		= {};
		uint16			_material_count = 0;
	};

	REGISTER_TRAIT(trait_model_instance, trait_types::trait_type_model_instance, trait_model_instance_reflection);
}
