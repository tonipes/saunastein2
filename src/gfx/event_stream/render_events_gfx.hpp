// Copyright (c) 2025 Inan Evin

#pragma once

#include "gfx/common/texture_buffer.hpp"
#include "gfx/common/gfx_common.hpp"
#include "gfx/common/descriptions.hpp"
#include "data/static_vector.hpp"
#include "math/vector2ui16.hpp"
#include "math/aabb.hpp"
#include "resources/primitive_raw.hpp"
#include "resources/common_resources.hpp"
#include "data/ostream.hpp"
#include "data/span.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "data/string.hpp"
#endif

namespace SFG
{
	class ostream;
	class istream;

	struct render_event_texture
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		string name = "";
#endif
		static_vector<texture_buffer, MAX_TEXTURE_MIPS> buffers;
		vector2ui16										size			  = {};
		uint32											intermediate_size = 0;
		uint8											format			  = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_sampler
	{
		sampler_desc desc = {};

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_shader
	{
		shader_desc desc  = {};
		uint8		flags = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_material
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		string name = "";
#endif
		uint8														 flags;
		uint8														 use_sampler   = 0;
		resource_id													 sampler_index = 0;
		span<uint8>													 data		   = {};
		static_vector<resource_handle, MAX_MATERIAL_TEXTURES>		 textures;
		static_vector<resource_handle, MAX_MATERIAL_SHADER_VARIANTS> shaders;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_model
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		string name = "";
#endif

		vector<resource_id> meshes;
		vector<resource_id> materials;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_mesh
	{
#ifndef SFG_STRIP_DEBUG_NAMES
		string name = "";
#endif

		aabb						  local_aabb;
		vector<primitive_static_raw>  primitives_static;
		vector<primitive_skinned_raw> primitives_skinned;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_material_reloaded
	{
	};

	struct render_event_resource_reloaded
	{
		resource_id prev_id;
		resource_id new_id;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct render_event_model_reloaded
	{
		resource_id			prev_id;
		resource_id			new_id;
		vector<resource_id> prev_meshes;
		vector<resource_id> new_meshes;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

}
