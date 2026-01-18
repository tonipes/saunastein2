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
#include "reflection/type_reflection.hpp"
#include "game/game_max_defines.hpp"

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{
	class ostream;
	class istream;
	class world;

	class comp_camera
	{
	public:
		static void reflect();

		// -----------------------------------------------------------------------------
		// trait
		// -----------------------------------------------------------------------------

		void on_add(world& w);
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

		void set_values(world& w, float near_plane, float far_plane, float fov_degrees, std::initializer_list<float> cascades = {0.01f, 0.075f, 0.12f, 0.25f});
		void set_values(world& w, float near_plane, float far_plane, float fov_degrees, const static_vector<float, MAX_SHADOW_CASCADES>& cascades);
		void set_main(world& w);

		inline float get_near() const
		{
			return _near;
		}

		inline float get_far() const
		{
			return _far;
		}

		inline float get_fov_degrees() const
		{
			return _fov_degrees;
		}
		inline const component_header& get_header() const
		{
			return _header;
		}

	private:
		template <typename T, int> friend class comp_cache;

	private:
		component_header _header = {};

		static_vector<float, MAX_SHADOW_CASCADES> _cascades;
		float									  _near		   = 0.1f;
		float									  _far		   = 0.1f;
		float									  _fov_degrees = 45.0f;
	};

	REFLECT_TYPE(comp_camera);

}
