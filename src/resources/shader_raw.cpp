// Copyright (c) 2025 Inan Evin

#include "shader_raw.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

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
		desc.serialize(stream);
	}

	void shader_raw::deserialize(istream& stream, bool use_embedded_layout, gfx_id layout)
	{
		stream >> name;
		stream >> source;
		stream >> defines;
		stream >> is_skinned;
		stream >> is_discard;
		desc.deserialize(stream);

		if (use_embedded_layout)
			desc.flags.set(shader_flags::shf_use_embedded_layout);
		else
			desc.layout = layout;

		SFG_INFO("Created shader from buffer: {0}", name);
	}

#ifdef SFG_TOOLMODE

	bool shader_raw::cook_from_file(const char* path, bool use_embedded_layout, gfx_id layout, bool use_working_dir)
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
			}

			const string& wd = engine_data::get().get_working_dir();
			const string  p	 = path;
			name			 = p.substr(wd.size(), p.size() - wd.size());

			const string full_source = use_working_dir ? (engine_data::get().get_working_dir() + source) : (source);
			if (!file_system::exists(full_source.c_str()))
			{
				SFG_ERR("File don't exist! {0}", path);
				return false;
			}
			const string shader_text = file_system::read_file_as_string(full_source.c_str());
			if (shader_text.empty())
				return false;

			desc.debug_name = name;
			desc.blobs		= {
				 {.stage = shader_stage::vertex},
				 {.stage = shader_stage::fragment},
			 };

			if (use_embedded_layout)
				desc.flags.set(shader_flags::shf_use_embedded_layout);
			else
				desc.layout = layout;

			span<uint8> layout_data	   = {};
			const bool	compile_layout = desc.flags.is_set(shader_flags::shf_use_embedded_layout);

			gfx_backend* backend	 = gfx_backend::get();
			const string folder_path = file_system::get_directory_of_file(full_source.c_str());
			if (!backend->compile_shader_vertex_pixel(shader_text, defines, folder_path.c_str(), desc.vertex_entry.c_str(), desc.pixel_entry.c_str(), desc.blobs[0].data, desc.blobs[1].data, compile_layout, desc.layout_data))
				return false;
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading shader: {0}", e.what());
			return false;
		}

		SFG_INFO("Created shader from file: {0}", name);
		return true;
	}
#endif
}
