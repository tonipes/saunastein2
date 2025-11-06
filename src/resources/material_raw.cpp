// Copyright (c) 2025 Inan Evin
#include "material_raw.hpp"
#include "data/istream.hpp"

#ifdef SFG_TOOLMODE
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"
#include "io/log.hpp"
#include "io/file_system.hpp"
#include "serialization/serialization.hpp"

#include "project/engine_data.hpp"
#include <fstream>
using json = nlohmann::json;
#endif
namespace SFG
{
	void material_raw::serialize(ostream& stream) const
	{
		const uint32 sz = static_cast<uint32>(material_data.get_size());
		stream << sz;

		if (sz != 0)
			stream.write_raw(material_data.get_raw(), material_data.get_size());

		stream << shader;
		stream << textures;
		stream << pass_mode;
		stream << name;
		stream << sid;
		stream << double_sided;
		stream << use_alpha_cutoff;
		stream << sampler_definitions;
	}

	void material_raw::deserialize(istream& stream)
	{
		uint32 sz = 0;
		stream >> sz;
		if (sz != 0)
		{
			material_data.create(static_cast<size_t>(sz));
			stream.read_to_raw(material_data.get_raw(), sz);
			material_data.shrink(static_cast<size_t>(sz));
		}

		stream >> shader;
		stream >> textures;
		stream >> pass_mode;
		stream >> name;
		stream >> sid;
		stream >> double_sided;
		stream >> use_alpha_cutoff;
		stream >> sampler_definitions;

		SFG_INFO("Created material from buffer: {0}", name);
	}

#ifdef SFG_TOOLMODE

	void to_json(nlohmann::json& j, const parameter_entry& p)
	{
		j = {{"name", p.name}, {"value", p.value}};
	}

	void from_json(const nlohmann::json& j, parameter_entry& p)
	{
		p.name	= j.at("name").get<std::string>();
		p.value = j.at("value");
	}

	bool material_raw::load_from_file(const char* relative_file, const char* base_path)
	{
		const string target_path = base_path + string(relative_file);
		if (!file_system::exists(target_path.c_str()))
		{
			SFG_ERR("File don't exist! {0}", target_path.c_str());
			return false;
		}

		try
		{
			std::ifstream f(target_path);
			json		  json_data = json::parse(f);
			f.close();

			const string pass = json_data.value<string>("pass", "gbuffer");

			if (pass.compare("gbuffer") == 0)
				pass_mode = material_pass_mode::gbuffer;
			else if (pass.compare("forward") == 0)
				pass_mode = material_pass_mode::forward;
			else
				pass_mode = material_pass_mode::gbuffer;

			sid					= TO_SID(relative_file);
			shader_path			= json_data.value<string>("shader", "");
			textures_path		= json_data.value<vector<string>>("textures", {});
			double_sided		= json_data.value<uint8>("double_sided", 0);
			use_alpha_cutoff	= json_data.value<uint8>("use_alpha_cutoff", 0);
			sampler_definitions = json_data.value<vector<sampler_desc>>("sampler_definitions", {});
			draw_priority		= json_data.value<uint16>("priority", 0);

			vector<parameter_entry> parameters;
			if (json_data.contains("parameters"))
				parameters = json_data.at("parameters").get<std::vector<parameter_entry>>();

			SFG_ASSERT(!shader_path.empty());

			name = relative_file;

			for (const string& txt : textures_path)
			{
				const string full = base_path + txt;
				if (!file_system::exists(full.c_str()))
				{
					SFG_ERR("File doesn't exists! {0}", full.c_str());
					return false;
				}
				textures.push_back(TO_SID(txt));
			}

			const string full_shader_path = base_path + shader_path;
			if (!file_system::exists(full_shader_path.c_str()))
			{
				SFG_ERR("File doesn't exists! {0}", full_shader_path.c_str());
				return false;
			}
			shader = TO_SID(shader_path);

			for (const parameter_entry& p : parameters)
			{
				const auto& param = p.value;

				if (param.is_number_unsigned())
				{
					uint32 val = param.get<uint32>();
					material_data << val;
				}
				else if (param.is_number_float())
				{
					float val = param.get<float>();
					material_data << val;
				}
				else if (param.is_object() && param.contains("x") && param.contains("y") && param.contains("z") && param.contains("w") && param.size() == 4)
				{
					vector4 val = param.get<vector4>();
					material_data << val;
				}
				else if (param.is_object() && param.contains("x") && param.contains("y") && param.contains("z") && param.size() == 3)
				{
					vector3 val = param.get<vector3>();
					material_data << val;
				}
				else if (param.is_object() && param.contains("x") && param.contains("y") && param.size() == 2)
				{
					vector2 val = param.get<vector2>();
					material_data << val;
				}
				else
				{
					SFG_ASSERT(false);
				}
			}
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading material: {0}", e.what());
			return false;
		}

		SFG_INFO("Created material from file: {0}", name);
		return true;
	}

	bool material_raw::load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension)
	{
		const string sid_str		 = std::to_string(TO_SID(relative_path));
		const string meta_cache_path = cache_folder_path + sid_str + "_meta" + extension;
		const string data_cache_path = cache_folder_path + sid_str + "_data" + extension;

		if (!file_system::exists(meta_cache_path.c_str()))
			return false;

		if (!file_system::exists(data_cache_path.c_str()))
			return false;

		istream stream = serialization::load_from_file(meta_cache_path.c_str());

		string file_path				= "";
		uint64 saved_file_last_modified = 0;
		stream >> file_path;
		stream >> saved_file_last_modified;

		stream.destroy();

		const uint64 file_last_modified = file_system::get_last_modified_ticks(file_path);

		if (file_last_modified != saved_file_last_modified)
			return false;

		stream = serialization::load_from_file(data_cache_path.c_str());
		deserialize(stream);
		stream.destroy();
		return true;
	}

	void material_raw::save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const
	{
		const string sid_str			= std::to_string(TO_SID(name));
		const string file_path			= resource_directory_path + name;
		const uint64 file_last_modified = file_system::get_last_modified_ticks(file_path);

		const string meta_cache_path = cache_folder_path + sid_str + "_meta" + extension;
		const string data_cache_path = cache_folder_path + sid_str + "_data" + extension;

		ostream out_stream;
		out_stream << file_path;
		out_stream << file_last_modified;
		serialization::save_to_file(meta_cache_path.c_str(), out_stream);

		out_stream.shrink(0);
		serialize(out_stream);
		serialization::save_to_file(data_cache_path.c_str(), out_stream);

		out_stream.destroy();
	}
#endif
}