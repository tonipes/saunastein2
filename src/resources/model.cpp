// Copyright (c) 2025 Inan Evin
#include "model.hpp"
#include "memory/chunk_allocator.hpp"
#include "mesh.hpp"
#include "model_node.hpp"
#include "skin.hpp"
#include "animation.hpp"
#include "model_raw.hpp"
#include "texture.hpp"
#include "material.hpp"
#include "gfx/world/world_renderer.hpp"
#include "project/engine_data.hpp"
#include "resource_reflection_template.hpp"

namespace SFG
{
	model_reflection g_model_reflection;

	model_reflection::model_reflection()
	{
		meta& m = reflection::get().register_meta(type_id<model>::value, type_id<model>::index, "stkmodel");

#ifdef SFG_TOOLMODE

		m.add_function<void*, const char*, world&>("cook_from_file"_hs, [](const char* path, world& w) -> void* {
			model_raw* raw = new model_raw();

			if (!raw->cook_from_file(path))
			{
				delete raw;
				return nullptr;
			}

			world_resources&				 resources = w.get_resources();
			world_resources::resource_watch& watch	   = resources.add_resource_watch();
			watch.base_path							   = path;
			watch.dependencies.push_back(engine_data::get().get_working_dir() + raw->name);

			return raw;
		});
#endif

		m.add_function<void*, istream&>("cook_from_stream"_hs, [](istream& stream) -> void* {
			model_raw* raw = new model_raw();
			raw->deserialize(stream);
			return raw;
		});

		m.add_function<resource_handle, void*, world&>("create_from_raw_2"_hs, [](void* raw, world& w) -> resource_handle {
			model_raw*		   raw_ptr	 = reinterpret_cast<model_raw*>(raw);
			world_resources&   resources = w.get_resources();
			resource_handle	   handle	 = resources.add_resource<model>(TO_SID(raw_ptr->name));
			model&			   res		 = resources.get_resource<model>(handle);
			chunk_allocator32& aux		 = resources.get_aux();

			res.create_from_raw(*raw_ptr, w.get_render_stream(), resources, aux);
			delete raw_ptr;

			resource_handle* mesh_handles = aux.get<resource_handle>(res.get_created_meshes());
			const uint16	 count		  = res.get_mesh_count();
			for (uint16 i = 0; i < count; i++)
			{
				mesh& created_mesh = resources.get_resource<mesh>(mesh_handles[i]);
			}

			return handle;
		});

		m.add_function<void, world&>("init_resource_storage"_hs, [](world& w) -> void { w.get_resources().init_storage<model>(MAX_WORLD_MODELS); });

		m.add_function<void, world&, resource_handle>("destroy"_hs, [](world& w, resource_handle h) -> void {
			world_resources& res = w.get_resources();
			res.get_resource<model>(h).destroy(res, w.get_render_stream(), res.get_aux());
			res.remove_resource<model>(h);
		});

		m.add_function<void, void*, ostream&>("serialize"_hs, [](void* loader, ostream& stream) -> void {
			model_raw* raw = reinterpret_cast<model_raw*>(loader);
			raw->serialize(stream);
		});
	}

	model::~model()
	{
		SFG_ASSERT(!_flags.is_set(model::flags::hw_exists));
	}

	void model::create_from_raw(const model_raw& raw, render_event_stream& stream, world_resources& resources, chunk_allocator32& alloc)
	{
		SFG_ASSERT(!_flags.is_set(model::flags::hw_exists));

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		_total_aabb = raw.total_aabb;

		const uint16 node_count		 = static_cast<uint16>(raw.loaded_nodes.size());
		const uint16 mesh_count		 = static_cast<uint16>(raw.loaded_meshes.size());
		const uint16 skins_count	 = static_cast<uint16>(raw.loaded_skins.size());
		const uint16 anims_count	 = static_cast<uint16>(raw.loaded_animations.size());
		const uint16 materials_count = static_cast<uint16>(raw.loaded_materials.size());
		const uint16 textures_count	 = static_cast<uint16>(raw.loaded_textures.size());

		if (node_count != 0)
		{
			_nodes				  = alloc.allocate<model_node>(node_count);
			model_node* ptr_nodes = alloc.get<model_node>(_nodes);

			for (uint16 i = 0; i < node_count; i++)
			{
				const model_node_raw& loaded_node = raw.loaded_nodes[i];
				model_node&			  node		  = ptr_nodes[i];
				node.create_from_raw(loaded_node, alloc);
			}
		}

		if (mesh_count != 0)
		{
			_created_meshes				= alloc.allocate<resource_handle>(mesh_count);
			resource_handle* meshes_ptr = alloc.get<resource_handle>(_created_meshes);

			for (uint16 i = 0; i < mesh_count; i++)
			{
				const mesh_raw&		  loaded_mesh = raw.loaded_meshes[i];
				const resource_handle handle	  = resources.add_resource<mesh>(loaded_mesh.sid);
				meshes_ptr[i]					  = handle;

				mesh& m = resources.get_resource<mesh>(handle);
				m.create_from_raw(loaded_mesh, alloc, stream, handle);
			}
		}

		if (skins_count != 0)
		{
			_created_skins			   = alloc.allocate<resource_handle>(skins_count);
			resource_handle* skins_ptr = alloc.get<resource_handle>(_created_skins);

			for (uint16 i = 0; i < skins_count; i++)
			{
				const skin_raw&		  loaded_skin = raw.loaded_skins[i];
				const resource_handle handle	  = resources.add_resource<skin>(loaded_skin.sid);
				skins_ptr[i]					  = handle;
				skin& created					  = resources.get_resource<skin>(handle);
				created.create_from_raw(loaded_skin, alloc);
			}
		}

		if (anims_count != 0)
		{
			_created_anims			   = alloc.allocate<resource_handle>(anims_count);
			resource_handle* anims_ptr = alloc.get<resource_handle>(_created_anims);

			for (uint16 i = 0; i < anims_count; i++)
			{
				const animation_raw&  loaded_anim = raw.loaded_animations[i];
				const resource_handle handle	  = resources.add_resource<animation>(loaded_anim.sid);
				anims_ptr[i]					  = handle;
				animation& created				  = resources.get_resource<animation>(handle);
				created.create_from_raw(loaded_anim, alloc);
			}
		}

		if (textures_count != 0)
		{
			_created_textures	 = alloc.allocate<resource_handle>(textures_count);
			resource_handle* ptr = alloc.get<resource_handle>(_created_textures);

			for (uint16 i = 0; i < textures_count; i++)
			{
				const texture_raw&	  loaded = raw.loaded_textures[i];
				const resource_handle handle = resources.add_resource<texture>(loaded.sid);
				ptr[i]						 = handle;
				texture& created			 = resources.get_resource<texture>(handle);
				created.create_from_raw(loaded, stream, alloc, handle);
			}
		}

		if (materials_count != 0)
		{
			_created_materials	 = alloc.allocate<resource_handle>(materials_count);
			resource_handle* ptr = alloc.get<resource_handle>(_created_materials);

			for (uint16 i = 0; i < materials_count; i++)
			{
				const material_raw&	  loaded = raw.loaded_materials[i];
				const resource_handle handle = resources.add_resource<material>(loaded.sid);
				ptr[i]						 = handle;
				material& created			 = resources.get_resource<material>(handle);
				created.create_from_raw(loaded, resources, resources.get_aux(), stream, handle);
			}
		}

		_skins_count	 = skins_count;
		_anims_count	 = anims_count;
		_meshes_count	 = mesh_count;
		_nodes_count	 = node_count;
		_materials_count = materials_count;
		_textures_count	 = textures_count;
		_flags.set(model::flags::pending_upload | model::flags::hw_exists);
	}

	void model::destroy(world_resources& resources, render_event_stream& stream, chunk_allocator32& alloc)
	{
		SFG_ASSERT(_flags.is_set(model::flags::hw_exists));

#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		if (_nodes.size != 0)
		{
			model_node* ptr_nodes = alloc.get<model_node>(_nodes);

			for (uint16 i = 0; i < _nodes_count; i++)
			{
				model_node& node = ptr_nodes[i];
				node.destroy(alloc);
			}

			alloc.free(_nodes);
		}

		if (_created_meshes.size != 0)
		{
			resource_handle* ptr = alloc.get<resource_handle>(_created_meshes);
			for (uint16 i = 0; i < _meshes_count; i++)
			{
				const resource_handle handle = ptr[i];
				if (!resources.is_valid<mesh>(handle))
					continue;

				mesh& m = resources.get_resource<mesh>(handle);
				m.destroy(alloc, stream, handle);
				resources.remove_resource<mesh>(handle);
			}

			alloc.free(_created_meshes);
		}

		if (_created_skins.size != 0)
		{
			resource_handle* skins_ptr = alloc.get<resource_handle>(_created_skins);

			for (uint16 i = 0; i < _skins_count; i++)
			{
				const resource_handle handle = skins_ptr[i];
				if (!resources.is_valid<skin>(handle))
					continue;
				skin& sk = resources.get_resource<skin>(handle);
				sk.destroy(alloc);
				resources.remove_resource<skin>(handle);
			}

			alloc.free(_created_skins);
		}

		if (_created_anims.size != 0)
		{
			resource_handle* anims_ptr = alloc.get<resource_handle>(_created_anims);

			for (uint16 i = 0; i < _anims_count; i++)
			{
				const resource_handle handle = anims_ptr[i];
				if (!resources.is_valid<animation>(handle))
					continue;
				animation& anim = resources.get_resource<animation>(handle);
				anim.destroy(alloc);
				resources.remove_resource<animation>(handle);
			}

			alloc.free(_created_anims);
		}

		if (_created_textures.size != 0)
		{
			resource_handle* ptr = alloc.get<resource_handle>(_created_textures);

			for (uint16 i = 0; i < _textures_count; i++)
			{
				const resource_handle handle = ptr[i];
				if (!resources.is_valid<texture>(handle))
					continue;
				texture& res = resources.get_resource<texture>(handle);
				res.destroy(stream, alloc, handle);
				resources.remove_resource<texture>(handle);
			}

			alloc.free(_created_textures);
		}

		if (_created_materials.size != 0)
		{
			resource_handle* ptr = alloc.get<resource_handle>(_created_materials);

			for (uint16 i = 0; i < _materials_count; i++)
			{
				const resource_handle handle = ptr[i];
				if (!resources.is_valid<material>(handle))
					continue;
				material& res = resources.get_resource<material>(handle);
				res.destroy(stream, alloc, handle);
				resources.remove_resource<material>(handle);
			}

			alloc.free(_created_materials);
		}

		_created_textures  = {};
		_created_materials = {};
		_created_meshes	   = {};
		_nodes			   = {};
		_created_skins	   = {};
		_created_anims	   = {};
		_flags.remove(model::flags::hw_exists);
	}
}