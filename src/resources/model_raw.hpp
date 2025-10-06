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

namespace SFG
{
	class ostream;
	class istream;

	struct model_raw
	{
		string				   name = "";
		vector<model_node_raw> loaded_nodes;
		vector<mesh_raw>	   loaded_meshes;
		vector<skin_raw>	   loaded_skins;
		vector<animation_raw>  loaded_animations;
		vector<texture_raw>	   loaded_textures;
		vector<material_raw>   loaded_materials;
		aabb				   total_aabb;
		vector<string_id>	   material_shaders;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

#ifdef SFG_TOOLMODE
		bool cook_from_file(const char* file, const char* relative_path);
		bool import_gtlf(const char* file, const char* relative_path, bool import_materials, bool import_textures);
#endif
	};
}
