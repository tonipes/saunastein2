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

#include "comp_audio.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "world/world.hpp"
#include "resources/audio.hpp"
#include "io/log.hpp"
#include <vendor/miniaudio/miniaudio.h>

namespace SFG
{
	void comp_audio::on_add(world& w)
	{
		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();
		_ma_sound			   = aux.allocate<ma_sound>(1);
	}

	void comp_audio::on_remove(world& w)
	{
		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();

		// uninit if inited
		if (!_audio_resource.is_null())
		{
			ma_sound* snd = aux.get<ma_sound>(_ma_sound);
			ma_sound_uninit(snd);
		}

		// dealloc sound
		aux.free(_ma_sound);
		_ma_sound		= {};
		_audio_resource = {};
	}

	void comp_audio::serialize(ostream& stream, world& w) const
	{
	}

	void comp_audio::deserialize(istream& stream, world& w)
	{
	}

	void comp_audio::play(world& w)
	{
		SFG_ASSERT(!_audio_resource.is_null());

		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();
		ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);

		ma_sound_start(snd);
	}

	void comp_audio::stop(world& w)
	{
		SFG_ASSERT(!_audio_resource.is_null());

		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();
		ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);
		ma_sound_stop(snd);
	}

	void comp_audio::reset(world& w)
	{
		SFG_ASSERT(!_audio_resource.is_null());

		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();
		ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);
		ma_sound_seek_to_pcm_frame(snd, 0);
	}

	void comp_audio::set_volume(world& w, float volume)
	{
		_volume = volume;

		if (!_audio_resource.is_null())
		{
			component_manager& cm  = w.get_comp_manager();
			chunk_allocator32& aux = cm.get_aux();
			ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);

			ma_sound_set_volume(snd, volume);
		}
	}

	void comp_audio::set_looping(world& w, uint8 looping)
	{
		_is_looping = looping;

		if (!_audio_resource.is_null())
		{
			component_manager& cm  = w.get_comp_manager();
			chunk_allocator32& aux = cm.get_aux();
			ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);

			ma_sound_set_looping(snd, _is_looping);
		}
	}

	void comp_audio::set_audio(world& w, resource_handle handle)
	{
		component_manager& cm  = w.get_comp_manager();
		resource_manager&  rm  = w.get_resource_manager();
		chunk_allocator32& aux = cm.get_aux();
		ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);

		if (!_audio_resource.is_null())
		{
			ma_sound_uninit(snd);
			_audio_resource = {};
		}

		_audio_resource = handle;
		audio& aud		= rm.get_resource<audio>(_audio_resource);

		ma_engine*		sound_engine = w.get_audio_manager().get_engine();
		const ma_uint32 flags		 = aud.get_flags().is_set(audio::flags::is_streaming) ? MA_SOUND_FLAG_STREAM : 0;
		const ma_result result		 = ma_sound_init_from_data_source(sound_engine, reinterpret_cast<ma_data_source*>(aud.get_decoder(w)), flags, nullptr, snd);
		if (result != MA_SUCCESS)
		{
			SFG_ERR("failed to init ma_sound for audio!");
			_audio_resource = {};
			return;
		}

		set_sound_params(w, snd);
	}

	void comp_audio::set_sound_params(world& w, ma_sound* snd)
	{
		ma_sound_set_volume(snd, _volume);
		ma_sound_set_looping(snd, _is_looping);
	}
}