// Copyright (c) 2025 Inan Evin

#include "trait_light.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_events_trait.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
	trait_light_reflection::trait_light_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<trait_light>::value, type_id<trait_light>::index, "");
		m.add_function<void, world&>("init_trait_storage"_hs, [](world& w) -> void { w.get_entity_manager().init_trait_storage<trait_light>(MAX_ENTITIES); });
	}

	void trait_light::on_add(world& w)
	{
		w.get_entity_manager().on_add_render_proxy(_header.entity);
	}

	void trait_light::on_remove(world& w)
	{
		w.get_entity_manager().on_remove_render_proxy(_header.entity);
	}

	void trait_light::serialize(ostream& stream, world& w) const
	{
	}

	void trait_light::deserialize(istream& stream, world& w)
	{
	}

#ifdef SFG_TOOLMODE

	void trait_light::serialize_json(nlohmann::json& j, world& w) const
	{
	}

	void trait_light::deserialize_json(nlohmann::json& j, world& w)
	{
	}

#endif
}