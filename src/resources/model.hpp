// Copyright (c) 2025 Inan Evin
#pragma once

#include "data/bitmask.hpp"
#include "math/aabb.hpp"
#include "memory/chunk_handle.hpp"
#include "resources/common_resources.hpp"
#include "reflection/resource_reflection.hpp"

namespace SFG
{
	struct model_raw;
	class world;

	class model
	{
	public:
		enum flags
		{
			created = 1 << 0,
		};

		~model();

		// -----------------------------------------------------------------------------
		// resource
		// -----------------------------------------------------------------------------

		void create_from_loader(const model_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline bitmask<uint8>& get_flags()
		{
			return _flags;
		}

		inline const aabb& get_total_aabb() const
		{
			return _total_aabb;
		}

		inline uint16 get_material_count() const
		{
			return _materials_count;
		}

		inline uint16 get_texture_count() const
		{
			return _textures_count;
		}

		inline uint16 get_node_count() const
		{
			return _nodes_count;
		}

		inline uint16 get_mesh_count() const
		{
			return _meshes_count;
		}

		inline uint16 get_light_count() const
		{
			return _lights_count;
		}

		inline chunk_handle32 get_created_meshes() const
		{
			return _created_meshes;
		}

		inline chunk_handle32 get_created_nodes() const
		{
			return _nodes;
		}

		inline chunk_handle32 get_created_materials() const
		{
			return _created_materials;
		}

		inline chunk_handle32 get_created_skins() const
		{
			return _created_skins;
		}

		inline chunk_handle32 get_created_anims() const
		{
			return _created_anims;
		}

		inline chunk_handle32 get_created_textures() const
		{
			return _created_textures;
		}

		inline chunk_handle32 get_created_lights() const
		{
			return _created_lights;
		}

	private:
#ifdef SFG_TOOLMODE
		friend struct model_raw;
#endif
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
		chunk_handle32 _nodes;
		chunk_handle32 _created_meshes;
		chunk_handle32 _created_skins;
		chunk_handle32 _created_anims;
		chunk_handle32 _created_materials;
		chunk_handle32 _created_textures;
		chunk_handle32 _created_lights;
		aabb		   _total_aabb;
		uint16		   _nodes_count		= 0;
		uint16		   _meshes_count	= 0;
		uint16		   _skins_count		= 0;
		uint16		   _anims_count		= 0;
		uint16		   _materials_count = 0;
		uint16		   _textures_count	= 0;
		uint16		   _lights_count	= 0;
		bitmask<uint8> _flags;
	};

	REGISTER_RESOURCE(model, "stkmodel");

}