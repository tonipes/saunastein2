// Copyright (c) 2025 Inan Evin

#include "render_events_gfx.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{
	void render_event_texture::serialize(ostream& stream) const
	{
		stream << size;
		stream << intermediate_size;
		stream << format;
		stream << static_cast<uint32>(buffers.size());

		for (const texture_buffer& b : buffers)
		{
			b.serialize(stream, true);
		}

#ifndef SFG_STRIP_DEBUG_NAMES
		stream << name;
#endif
	}

	void render_event_texture::deserialize(istream& stream)
	{
		uint32 sz = 0;
		stream >> size;
		stream >> intermediate_size;
		stream >> format;
		stream >> sz;
		buffers.resize(sz);
		for (uint32 i = 0; i < sz; i++)
		{
			texture_buffer& b = buffers[i];
			b.deserialize(stream, true);
		}

#ifndef SFG_STRIP_DEBUG_NAMES
		stream >> name;
#endif
	}

	void render_event_sampler::serialize(ostream& stream) const
	{
		desc.serialize(stream);
	}

	void render_event_sampler::deserialize(istream& stream)
	{
		desc.deserialize(stream);
	}

	void render_event_shader::serialize(ostream& stream) const
	{
		desc.serialize(stream, true);
		stream << flags;
	}

	void render_event_shader::deserialize(istream& stream)
	{
		desc.deserialize(stream, true);
		stream >> flags;
	}

	void render_event_material::serialize(ostream& stream) const
	{
		const uint32 txt_sz		= static_cast<uint32>(textures.size());
		const uint32 shaders_sz = static_cast<uint32>(shaders.size());
		stream << txt_sz;
		stream << shaders_sz;
		stream << flags;

		for (resource_handle h : textures)
		{
			stream << h.index;
			stream << h.generation;
		}

		for (resource_handle h : shaders)
		{
			stream << h.index;
			stream << h.generation;
		}

#ifndef SFG_STRIP_DEBUG_NAMES
		stream << name;
#endif

		stream << static_cast<uint32>(data.size);
		stream.write_raw(data.data, data.size);
	}

	void render_event_material::deserialize(istream& stream)
	{
		uint32 txt_sz	  = 0;
		uint32 shaders_sz = 0;
		stream >> txt_sz;
		stream >> shaders_sz;
		stream >> flags;
		textures.resize(txt_sz);
		shaders.resize(shaders_sz);

		for (uint32 i = 0; i < txt_sz; i++)
		{
			resource_handle& h = textures[i];
			stream >> h.index;
			stream >> h.generation;
		}

		for (uint32 i = 0; i < shaders_sz; i++)
		{
			resource_handle& h = shaders[i];
			stream >> h.index;
			stream >> h.generation;
		}

#ifndef SFG_STRIP_DEBUG_NAMES
		stream >> name;
#endif

		uint32 sz = 0;
		stream >> sz;
		data.size = sz;
		data.data = stream.get_data_current();
		stream.skip_by(sz);
	}

	void render_event_mesh::serialize(ostream& stream) const
	{
		stream << primitives_static;
		stream << primitives_skinned;
		stream << local_aabb;

#ifndef SFG_STRIP_DEBUG_NAMES
		stream << name;
#endif
	}

	void render_event_mesh::deserialize(istream& stream)
	{
		stream >> primitives_static;
		stream >> primitives_skinned;
		stream >> local_aabb;

#ifndef SFG_STRIP_DEBUG_NAMES
		stream >> name;
#endif
	}

	void render_event_model::serialize(ostream& stream) const
	{
		stream << meshes;
		stream << materials;

#ifndef SFG_STRIP_DEBUG_NAMES
		stream << name;
#endif
	}

	void render_event_model::deserialize(istream& stream)
	{
		stream >> meshes;
		stream >> materials;

#ifndef SFG_STRIP_DEBUG_NAMES
		stream >> name;
#endif
	}
}
