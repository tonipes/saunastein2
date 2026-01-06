/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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

	class comp_model_instance
	{
	public:
		typedef void (*on_instantiated)(comp_model_instance* inst, resource_handle model, void* user_data);

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
		// impl
		// -----------------------------------------------------------------------------

		void instantiate_model_to_world(world& w, resource_handle model);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline resource_handle get_model() const
		{
			return _target_model;
		}

		inline const component_header& get_header() const
		{
			return _header;
		}

		inline void set_instantiate_callback(on_instantiated cb, void* user_data)
		{
			_instantiate_callback  = cb;
			_instantiate_user_data = user_data;
		}

	private:
		template <typename T, int> friend class comp_cache;

		void fetch_refs(resource_manager& res, string_id& out_target) const;
		void fill_refs(resource_manager& res, string_id target);

	private:
		on_instantiated _instantiate_callback  = nullptr;
		void*			_instantiate_user_data = nullptr;
		component_header _header				   = {};
		resource_handle _target_model		   = {};
		chunk_handle32	_root_entities		   = {};
		chunk_handle32	_skin_entities		   = {};
		uint16			_skin_entities_count   = 0;
		uint32			_root_entities_count   = 0;
	};

	REGISTER_TRAIT(comp_model_instance);
}
