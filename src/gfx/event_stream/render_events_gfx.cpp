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
		stream << layout;
		stream << pso_variants;

		const uint32 compile_variants_sz = static_cast<uint32>(compile_variants.size());
		stream << compile_variants_sz;

		for (const compile_variant& v : compile_variants)
		{
			v.serialize(stream, true);
		}
	}

	void render_event_shader::deserialize(istream& stream)
	{
		stream >> layout;
		stream >> pso_variants;

		uint32 compile_variants_size = 0;
		stream >> compile_variants_size;

		compile_variants.resize(compile_variants_size);
		for (uint32 i = 0; i < compile_variants_size; i++)
		{
			compile_variant& v = compile_variants[i];
			v.deserialize(stream, true);
		}
	}

	void render_event_material::serialize(ostream& stream) const
	{
		const uint32 txt_sz = static_cast<uint32>(textures.size());
		const uint32 smp_sz = static_cast<uint32>(samplers.size());
		stream << txt_sz;
		stream << smp_sz;
		stream << flags;
		stream << shader_index;
		stream << priority;

		for (resource_handle h : textures)
		{
			stream << h.index;
			stream << h.generation;
		}

		for (resource_handle h : samplers)
		{
			stream << h.index;
			stream << h.generation;
		}

#ifndef SFG_STRIP_DEBUG_NAMES
		stream << name;
#endif

		const uint64 addr = reinterpret_cast<uint64>(data.data);
		stream << static_cast<uint32>(data.size);
		stream << addr;
	}

	void render_event_material::deserialize(istream& stream)
	{
		uint32 txt_sz = 0;
		uint32 smp_sz = 0;
		stream >> txt_sz;
		stream >> smp_sz;
		stream >> flags;
		stream >> shader_index;
		stream >> priority;
		textures.resize(txt_sz);
		samplers.resize(smp_sz);

		for (uint32 i = 0; i < txt_sz; i++)
		{
			resource_handle& h = textures[i];
			stream >> h.index;
			stream >> h.generation;
		}

		for (uint32 i = 0; i < smp_sz; i++)
		{
			resource_handle& h = samplers[i];
			stream >> h.index;
			stream >> h.generation;
		}

#ifndef SFG_STRIP_DEBUG_NAMES
		stream >> name;
#endif

		uint64 addr = 0;
		uint32 sz	= 0;
		stream >> sz;
		stream >> addr;
		data.size = sz;
		data.data = reinterpret_cast<uint8*>(addr);
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

	void render_event_resource_reloaded::serialize(ostream& stream) const
	{
		stream << prev_id;
		stream << new_id;
	}

	void render_event_resource_reloaded::deserialize(istream& stream)
	{
		stream >> prev_id;
		stream >> new_id;
	}

	void render_event_model_update_materials::serialize(ostream& stream) const
	{
		stream << materials;
	}

	void render_event_model_update_materials::deserialize(istream& stream)
	{
		stream >> materials;
	}

}
