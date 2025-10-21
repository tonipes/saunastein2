// Copyright (c) 2025 Inan Evin

#pragma once

#include "data/ostream.hpp"
#include "data/static_vector.hpp"
#include "common/string_id.hpp"
#include "resources/common_resources.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/descriptions.hpp"

#ifdef SFG_TOOLMODE
#include "data/string.hpp"
#include "vendor/nhlohmann/json.hpp"
#endif

namespace SFG
{
	class istream;

#ifdef SFG_TOOLMODE
	struct parameter_entry
	{
		string		   name;
		nlohmann::json value;
	};

	void to_json(nlohmann::json& j, const parameter_entry& p);
	void from_json(const nlohmann::json& j, parameter_entry& p);

#endif

	enum class material_pass_mode
	{
		gbuffer,
		gbuffer_transparent,
		forward,
	};

	struct material_raw
	{
		ostream			   material_data = {};
		vector<string_id>  shaders;
		vector<string_id>  textures;
		vector<string>	   shaders_path;
		vector<string>	   textures_path;
		material_pass_mode pass_mode;
		string			   name					  = "";
		string_id		   sid					  = 0;
		sampler_desc	   sampler_definition	  = {};
		uint8			   use_sampler_definition = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

#ifdef SFG_TOOLMODE
		bool load_from_file(const char* file);
		void get_dependencies(vector<string>& out_deps) const {};
#endif
	};
}
