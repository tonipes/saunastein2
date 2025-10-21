// Copyright (c) 2025 Inan Evin
#pragma once

#include "math/aabb.hpp"
#include "data/vector.hpp"
#include "animation_raw.hpp"
#include "skin_raw.hpp"
#include "mesh_raw.hpp"
#include "material_raw.hpp"
#include "texture_raw.hpp"
#include "model_node_raw.hpp"
#include "light_raw.hpp"
#include "texture_sampler_raw.hpp"

namespace SFG
{
	class ostream;
	class istream;

	struct model_raw
	{
		string				   name	  = "";
		string				   source = "";
		vector<model_node_raw> loaded_nodes;
		vector<mesh_raw>	   loaded_meshes;
		vector<skin_raw>	   loaded_skins;
		vector<animation_raw>  loaded_animations;
		vector<texture_raw>	   loaded_textures;
		vector<material_raw>   loaded_materials;
		vector<light_raw>	   loaded_lights;
		aabb				   total_aabb;
		vector<string_id>	   material_shaders;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

#ifdef SFG_TOOLMODE
		bool load_from_file(const char* file);
		void get_dependencies(vector<string>& out_deps) const;
		bool import_gtlf(const char* file, const char* relative_path, bool import_materials, bool import_textures);
#endif
	};
}
