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

#include "render_event_stream.hpp"
#include "data/istream.hpp"
#include <tracy/Tracy.hpp>

namespace SFG
{
	void render_event_stream::init()
	{
		_main_thread_data.create(RENDER_STREAM_BATCH_SIZE);

		for (uint32 i = 0; i < RENDER_STREAM_MAX_BATCHES; i++)
		{
			_stream_data[i].data		   = new uint8[RENDER_STREAM_BATCH_SIZE];
			_proxy_entity_data[i].entities = new proxy_entity_transform_data[MAX_ENTITIES];
			_proxy_entity_data[i].dirty_indices.reserve(MAX_ENTITIES);
			_proxy_entity_data[i].dirty_flags = new uint8[MAX_ENTITIES];
			SFG_MEMSET(_proxy_entity_data[i].dirty_flags, 0, sizeof(uint8) * MAX_ENTITIES);
		}
	}

	void render_event_stream::uninit()
	{
		_main_thread_data.destroy();

		for (uint32 i = 0; i < RENDER_STREAM_MAX_BATCHES; i++)
		{
			delete _stream_data[i].data;
			delete _proxy_entity_data[i].entities;
			delete _proxy_entity_data[i].dirty_flags;
			_stream_data[i].data			  = nullptr;
			_proxy_entity_data[i].entities	  = nullptr;
			_proxy_entity_data[i].dirty_flags = nullptr;
			_proxy_entity_data[i].dirty_indices.resize(0);
		}
	}

	void render_event_stream::publish()
	{
		ZoneScoped;

		const size_t sz = _main_thread_data.get_size();
		if (sz == 0)
			return;
		SFG_ASSERT(sz < RENDER_STREAM_BATCH_SIZE);

		const int8 last_rendered = _rendered.load(std::memory_order_acquire);
		_write_index			 = (last_rendered + 1) % RENDER_STREAM_MAX_BATCHES;

		buffered_data& buf	   = _stream_data[_write_index];
		const size_t   data_sz = _main_thread_data.get_size();

		if (buf.size + data_sz >= RENDER_STREAM_BATCH_SIZE)
		{
			SFG_FATAL("writing too much data to event_stream data! grow RENDER_STREAM_BATCH_SIZE!");
			return;
		}

		SFG_ASSERT(buf.size + data_sz < RENDER_STREAM_BATCH_SIZE);
		SFG_MEMCPY(buf.data + buf.size, _main_thread_data.get_raw(), data_sz);
		buf.size += data_sz;

		_main_thread_data.shrink(0);
		_latest.store(_write_index, std::memory_order_release);
	}

	void render_event_stream::add_entity_transform_event(world_id index, const matrix4x3& model, const quat& rot)
	{
		proxy_entity_data& ped = _proxy_entity_data[_write_index];
		ped.peak_size		   = index >= ped.peak_size ? (index + 1) : ped.peak_size;

		proxy_entity_transform_data& e = ped.entities[index];
		e.rot						   = rot;
		e.model						   = model;

		uint8& dirty = ped.dirty_flags[index];
		if (dirty == 0)
		{
			ped.dirty_indices.push_back(index);
			dirty = 1;
		}
	}

	void render_event_stream::read_transform_events(render_proxy_entity* out_entities, uint32& out_size)
	{
		ZoneScoped;

		const int8 last_written = _latest.load(std::memory_order_acquire);
		if (last_written < 0)
			return;

		proxy_entity_data& ped = _proxy_entity_data[last_written];

		for (world_id d : ped.dirty_indices)
		{
			proxy_entity_transform_data& e = ped.entities[d];

			render_proxy_entity& write = out_entities[d];
			write.rotation			   = e.rot;
			write.model				   = e.model;
			write.normal			   = e.model.to_linear3x3().inversed().transposed();
			write.status			   = render_proxy_status::rps_active;
		}

		out_size = ped.peak_size;
		ped.dirty_indices.resize(0);
		SFG_MEMSET(ped.dirty_flags, 0, sizeof(uint8) * ped.peak_size);
	}

	void render_event_stream::open_into(istream& stream)
	{
		const int8 last_written = _latest.load(std::memory_order_acquire);
		if (last_written < 0)
			return;

		_rendered.store(last_written, std::memory_order_release);
		buffered_data& buf = _stream_data[last_written];
		stream.open(buf.data, buf.size);
		buf.size = 0;
	}

}