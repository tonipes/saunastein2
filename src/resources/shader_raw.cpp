// Copyright (c) 2025 Inan Evin

#include "shader_raw.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "gfx/renderer.hpp"

#ifdef SFG_TOOLMODE
#include "io/file_system.hpp"
#include "project/engine_data.hpp"
#include "gfx/backend/backend.hpp"
#include "vendor/nhlohmann/json.hpp"
#include <fstream>
using json = nlohmann::json;
#endif

namespace SFG
{
	void shader_raw::destroy()
	{
		desc.destroy();
	}

	void shader_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << source;
		stream << defines;
		stream << is_skinned;
		stream << is_discard;
		stream << is_z_prepass;
		desc.serialize(stream);
	}

	void shader_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> source;
		stream >> defines;
		stream >> is_skinned;
		stream >> is_discard;
		stream >> is_z_prepass;
		desc.deserialize(stream);

		desc.layout = renderer::get_bind_layout_global();
		SFG_INFO("Created shader from buffer: {0}", name);
	}

#ifdef SFG_TOOLMODE

	bool shader_raw::load_from_file(const char* path)
	{
		return load_from_file(path, false, renderer::get_bind_layout_global(), nullptr);
	}

	bool shader_raw::load_from_file(const char* path, bool use_embedded_layout, gfx_id layout, const char* base_directory_for_source)
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

			source	= json_data.value<string>("source", "");
			desc	= json_data.value<shader_desc>("desc", {});
			defines = json_data.value<vector<string>>("defines", {});

			for (const string& def : defines)
			{
				if (def.compare("USE_SKINNING") == 0)
					is_skinned = 1;

				if (def.compare("USE_DISCARD") == 0)
					is_discard = 1;

				if (def.compare("ZPREPASS") == 0)
					is_z_prepass = 1;
			}

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
				return false;

			desc.debug_name = name;
			desc.layout		= renderer::get_bind_layout_global();

			span<uint8> layout_data	   = {};
			const bool	compile_layout = desc.flags.is_set(shader_flags::shf_use_embedded_layout);

			gfx_backend* backend	 = gfx_backend::get();
			const string folder_path = file_system::get_directory_of_file(full_source.c_str());

			if (!desc.vertex_entry.empty())
			{
				desc.blobs.push_back({.stage = shader_stage::vertex});
				shader_blob& blob = desc.blobs.back();
				if (!backend->compile_shader_vertex_pixel(shader_stage::vertex, shader_text, defines, folder_path.c_str(), desc.vertex_entry.c_str(), blob.data, compile_layout, desc.layout_data))
					return false;
			}

			if (!desc.pixel_entry.empty())
			{
				desc.blobs.push_back({.stage = shader_stage::fragment});
				shader_blob& blob = desc.blobs.back();
				if (!backend->compile_shader_vertex_pixel(shader_stage::fragment, shader_text, defines, folder_path.c_str(), desc.pixel_entry.c_str(), blob.data, compile_layout, desc.layout_data))
					return false;
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
}
