// Copyright (c) 2025 Inan Evin
#pragma once

#include "world/components/common_comps.hpp"
#include "reflection/component_reflection.hpp"
#include "resources/common_resources.hpp"
#include "memory/chunk_handle.hpp"

struct ma_sound;

namespace SFG
{
	class ostream;
	class istream;
	class world;

	class comp_audio
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
		// impl
		// -----------------------------------------------------------------------------

		void play(world& w);
		void stop(world& w);
		void reset(world& w);
		void set_volume(world& w, float volume);
		void set_looping(world& w, uint8 looping);
		void set_audio(world& w, resource_handle handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const component_header& get_header() const
		{
			return _header;
		}

	private:
		void set_sound_params(world& w, ma_sound* snd);

	private:
		template <typename T, int> friend class comp_cache;

	private:
		component_header _header		 = {};
		resource_handle	 _audio_resource = {};
		chunk_handle32	 _ma_sound		 = {};
		float			 _volume		 = 1.0f;
		uint8			 _is_looping	 = 0;
	};

	REGISTER_TRAIT(comp_audio);
}
