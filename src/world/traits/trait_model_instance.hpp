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

	class trait_model_instance
	{
	public:
		void on_add(world& we);
		void on_remove(world& w);
		void set_model(resource_handle model);
		void instantiate_model_to_world(world& w, resource_handle model, resource_handle* materials = nullptr, uint32 materials_count = 0);

		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

#ifdef SFG_TOOLMODE
		void serialize_json(nlohmann::json& j, world& w) const;
		void deserialize_json(const nlohmann::json& j, world& w);
#endif

		inline resource_handle get_model() const
		{
			return _target_model;
		}

		inline const trait_header& get_header() const
		{
			return _header;
		}

	private:
		template <typename T, int> friend class trait_cache;

		void fetch_refs(resource_manager& res, string_id& out_target) const;
		void fill_refs(resource_manager& res, string_id target);

	private:
		trait_header	_header				 = {};
		resource_handle _target_model		 = {};
		chunk_handle32	_root_entities		 = {};
		uint32			_root_entities_count = 0;
	};

	REGISTER_TRAIT(trait_model_instance);
}
