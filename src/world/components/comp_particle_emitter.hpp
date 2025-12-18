// Copyright (c) 2025 Inan Evin
#pragma once

#include "world/components/common_comps.hpp"
#include "reflection/component_reflection.hpp"
#include "resources/common_resources.hpp"
#include "memory/pool_handle.hpp"

namespace SFG
{
	class ostream;
	class istream;
	class world;

	class comp_particle_emitter
	{
	public:
		// -----------------------------------------------------------------------------
		// trait
		// -----------------------------------------------------------------------------

		void on_add(world& w);
		void on_remove(world& w);
		void serialize(ostream& stream, world& w) const;
		void deserialize(istream& stream, world& w);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const component_header& get_header() const
		{
			return _header;
		}

		inline pool_handle16 get_state_machine() const
		{
			return _state_machine;
		}

	private:
		template <typename T, int> friend class comp_cache;

	private:
		component_header _header		= {};
		pool_handle16	 _state_machine = {};
	};

	REGISTER_TRAIT(comp_particle_emitter);
}
