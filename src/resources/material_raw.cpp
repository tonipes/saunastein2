// Copyright (c) 2025 Inan Evin
#include "material_raw.hpp"
#include "data/istream.hpp"

#ifdef SFG_TOOLMODE
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"
#include "io/log.hpp"
#include "io/file_system.hpp"
#include "project/engine_data.hpp"
#include <fstream>
using json = nlohmann::json;
#endif
namespace SFG
{
	ostream			  material_data = {};
	vector<string_id> shaders;
	vector<string_id> textures;
	uint8			  is_opaque	 = 0;
	uint8			  is_forward = 0;

	void material_raw::serialize(ostream& stream) const
	{
		const uint32 sz = static_cast<uint32>(material_data.get_size());
		stream << sz;

		if (sz != 0)
			stream.write_raw(material_data.get_raw(), material_data.get_size());

		stream << shaders;
		stream << textures;
		stream << pass_mode;
		stream << name;
		stream << sid;
		stream << use_sampler_definition;
		sampler_definition.serialize(stream);
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

		stream >> shaders;
		stream >> textures;
		stream >> pass_mode;
		stream >> name;
		stream >> sid;
		stream >> use_sampler_definition;
		sampler_definition.deserialize(stream);

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

	bool material_raw::cook_from_file(const char* file)
	{
		if (!file_system::exists(file))
		{
			SFG_ERR("File doesn't exists! {0}", file);
			return false;
		}

		try
		{
			std::ifstream f(file);
			json		  json_data = json::parse(f);
			f.close();

			const string pass = json_data.value<string>("pass", "gbuffer");

			if (pass.compare("gbuffer_transparent") == 0)
				pass_mode = material_pass_mode::gbuffer_transparent;
			else if (pass.compare("forward"))
				pass_mode = material_pass_mode::gbuffer_transparent;
			else
				pass_mode = material_pass_mode::gbuffer;

			sid			  = TO_SID(file);
			shaders_path  = json_data.value<vector<string>>("shaders", {});
			textures_path = json_data.value<vector<string>>("textures", {});

			vector<parameter_entry> parameters;
			if (json_data.contains("parameters"))
				parameters = json_data.at("parameters").get<std::vector<parameter_entry>>();

			SFG_ASSERT(!shaders_path.empty());

			const string& wd = engine_data::get().get_working_dir();
			const string  p	 = file;
			name			 = p.substr(wd.size(), p.size() - wd.size());

			const string engine_path = engine_data::get().get_working_dir();

			for (const string& txt : textures_path)
			{
				const string full = engine_path + txt;
				if (!file_system::exists(full.c_str()))
				{
					SFG_ERR("File doesn't exists! {0}", full.c_str());
					return false;
				}
				textures.push_back(TO_SID(txt));
			}

			for (const string& sh : shaders_path)
			{
				const string full = engine_path + sh;
				if (!file_system::exists(full.c_str()))
				{
					SFG_ERR("File doesn't exists! {0}", full.c_str());
					return false;
				}
				shaders.push_back(TO_SID(sh));
			}

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
#endif
}