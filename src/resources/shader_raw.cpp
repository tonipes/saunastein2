// Copyright (c) 2025 Inan Evin

#include "shader_raw.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

#ifdef SFG_TOOLMODE
#include "io/file_system.hpp"
#include "project/engine_data.hpp"
#include "gfx/backend/backend.hpp"
#include "shader_variant_compiler.hpp"
#include "vendor/nhlohmann/json.hpp"
#include <fstream>
using json = nlohmann::json;
#endif

namespace SFG
{

#ifdef SFG_TOOLMODE

	bool shader_raw::compile_specialized(const string& shader_text, const string& folder_path, const string& variant_style)
	{
		if (variant_style.compare("gbuffer_object") == 0)
			return shader_variant_compiler::compile_style_gbuffer_object(*this, shader_text, folder_path);

		SFG_ERR("Unrecognize shader variant style: {0} {1}", variant_style, name);
		return false;
	}

	bool shader_raw::load_from_file(const char* path)
	{
		return load_from_file(path, nullptr);
	}

	bool shader_raw::load_from_file(const char* path, const char* base_directory_for_source)
	{
		if (!file_system::exists(path))
		{
			SFG_ERR("File don't exist! {0}", path);
			return false;
		}
		try
		{
			std::ifstream f(path);
			json		  json_data = json::parse(f);
			f.close();

			// Receive & verify source.
			source			 = json_data.value<string>("source", "");
			const string& wd = engine_data::get().get_working_dir();
			const string  p	 = path;
			name			 = p.substr(wd.size(), p.size() - wd.size());

			const string full_source = base_directory_for_source == nullptr ? (engine_data::get().get_working_dir() + source) : (string(base_directory_for_source) + source);
			if (!file_system::exists(full_source.c_str()))
			{
				SFG_ERR("File don't exist! {0}", full_source.c_str());
				return false;
			}

			const string shader_text = file_system::read_file_as_string(full_source.c_str());
			if (shader_text.empty())
			{
				SFG_ERR("Failed reading shader text! {0}", full_source.c_str());
				return false;
			}
			const string folder_path = file_system::get_directory_of_file(full_source.c_str());

			if (json_data.find("desc") != json_data.end())
			{
				const shader_desc default_desc = json_data.value<shader_desc>("desc", {});
				return shader_variant_compiler::compile_raw(*this, shader_text, folder_path, default_desc);
			}
			else
			{
				if (json_data.find("variant_style") == json_data.end())
				{
					SFG_ERR("Failed compiling shader as it does not define a raw description neither a variant style! {0}", path);
					return false;
				}

				const string variant_style = json_data.value<string>("variant_style", "");
				return compile_specialized(shader_text, folder_path, variant_style);
			}
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading shader: {0}", e.what());
			return false;
		}

		SFG_INFO("Created shader from file: {0}", name);
		return true;
	}
	void shader_raw::get_dependencies(vector<string>& out_deps) const
	{
		out_deps.push_back(source);
	}
#endif

	void shader_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << source;
		stream << compile_variants;
		stream << pso_variants;
	}

	void shader_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> source;
		stream >> compile_variants;
		stream >> pso_variants;

		SFG_INFO("Created shader from buffer: {0}", name);
	}

	void shader_raw::destroy()
	{
		for (compile_variant& cv : compile_variants)
			cv.destroy();
	}

}
