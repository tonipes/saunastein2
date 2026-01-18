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
#include "resources/common_resources.hpp"
#include "memory/chunk_handle.hpp"

struct ma_sound;

namespace SFG
{
	class ostream;
	class istream;
	class world;
	class vector3;

	enum class sound_attenuation : uint8
	{
		none,
		inverse,
		linear,
		exponential,
	};

	class comp_audio
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

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void play(world& w);
		void toggle_play(world& w);
		void stop(world& w);
		void reset(world& w);
		void set_volume(world& w, float volume);
		void set_looping(world& w, uint8 looping);
		void set_audio(world& w, resource_handle handle);
		void set_attenuation_params(world& w, sound_attenuation att, float min_radius = 0.0f, float max_radius = 10.0f, float rolloff = 1.0f);
		void set_audio_position(world& w, const vector3& p);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const component_header& get_header() const
		{
			return _header;
		}

		inline bool is_play_on_start() const
		{
			return _play_on_start;
		}

		inline float get_radius_min() const
		{
			return _radius_min;
		}

		inline float get_radius_max() const
		{
			return _radius_max;
		}

		inline float get_rolloff() const
		{
			return _rolloff;
		}
		inline sound_attenuation get_attenuation() const
		{
			return _attenuation;
		}

	private:
		void set_sound_params(world& w, ma_sound* snd);

	private:
		template <typename T, int> friend class comp_cache;

	private:
		component_header  _header		  = {};
		resource_handle	  _audio_resource = {};
		chunk_handle32	  _ma_sound		  = {};
		float			  _rolloff		  = 0.0f;
		float			  _radius_min	  = 0.0f;
		float			  _radius_max	  = 10.0f;
		float			  _volume		  = 1.0f;
		sound_attenuation _attenuation	  = sound_attenuation::none;
		bool			  _is_looping	  = true;
		bool			  _play_on_start  = false;
	};

	REFLECT_TYPE(comp_audio);
}
