/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "entity_template_raw.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "data/hash_map.hpp"

#ifdef SFG_TOOLMODE
#include "reflection/reflection.hpp"
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "world/common_entity.hpp"
#include "world/component_manager.hpp"
#include "math/color.hpp"
#include "serialization/serialization.hpp"
#include <fstream>
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
	void entity_template_entity_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << position;
		stream << rotation;
		stream << scale;
		stream << parent;
		stream << first_child;
		stream << next_sibling;
		stream << visible;
	}

	void entity_template_entity_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> position;
		stream >> rotation;
		stream >> scale;
		stream >> parent;
		stream >> first_child;
		stream >> next_sibling;
		stream >> visible;
	}

#ifdef SFG_TOOLMODE
	void to_json(nlohmann::json& j, const entity_template_entity_raw& r)
	{
		j["name"]		  = r.name;
		j["pos"]		  = r.position;
		j["rot"]		  = r.rotation;
		j["scale"]		  = r.scale;
		j["visible"]	  = r.visible;
		j["parent"]		  = r.parent;
		j["first_child"]  = r.first_child;
		j["next_sibling"] = r.next_sibling;
	}

	void from_json(const nlohmann::json& j, entity_template_entity_raw& r)
	{
		r.name		   = j.value<string>("name", "");
		r.position	   = j.value<vector3>("pos", vector3::zero);
		r.rotation	   = j.value<quat>("rot", quat::identity);
		r.scale		   = j.value<vector3>("scale", vector3::one);
		r.visible	   = j.value<uint8>("visible", 1);
		r.parent	   = j.value<int32>("parent", -1);
		r.first_child  = j.value<int32>("first_child", -1);
		r.next_sibling = j.value<int32>("next_sibling", -1);
	}

#endif

	void entity_template_raw::serialize(ostream& stream) const
	{
		stream << entities;
		const uint32 sz = static_cast<uint32>(component_buffer.get_size());
		stream << sz;
		stream.write_raw(component_buffer.get_raw(), sz);
	}

	void entity_template_raw::deserialize(istream& stream)
	{
		stream >> entities;
		uint32 sz = 0;
		stream >> sz;
		if (sz > 0)
		{
			component_buffer.create(sz);
			stream.read_to_raw(component_buffer.get_raw(), sz);
			component_buffer.shrink(sz);
		}
	}

	void entity_template_raw::destroy()
	{
		if (component_buffer.get_size() != 0)
			component_buffer.destroy();
	}

#ifdef SFG_TOOLMODE

	void entity_template_raw::save_to_file(const char* path, world& w, const vector<world_handle>& handles)
	{
		json j = {};
		save_to_json(j, w, handles);

		std::ofstream file(path);
		if (file.is_open())
		{
			file << j.dump(4);
			file.close();
		}
	}

	void entity_template_raw::save_to_json(nlohmann::json& j, world& w, const vector<world_handle>& handles)
	{
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();
		resource_manager&  rm = w.get_resource_manager();

		vector<world_handle> order;
		order.reserve(512);
		auto collect = [&](world_handle h) { order.push_back(h); };

		for (world_handle h : handles)
		{
			collect(h);
			em.visit_children(h, collect);
		}

		hash_map<uint32, int32> index_by_world;
		index_by_world.reserve(order.size());
		for (size_t i = 0; i < order.size(); ++i)
			index_by_world[order[i].index] = static_cast<int32>(i);

		j["entities"]	= json::array();
		j["components"] = json::array();

		json& je = j["entities"];
		json& jc = j["components"];

		for (size_t i = 0; i < order.size(); i++)
		{
			const world_handle h	= order[i];
			const entity_meta& meta = em.get_entity_meta(h);

			entity_template_entity_raw er = {};
			er.name						  = meta.name;
			er.position					  = em.get_entity_position(h);
			er.scale					  = em.get_entity_scale(h);
			er.rotation					  = em.get_entity_rotation(h);
			er.parent					  = -1;
			er.first_child				  = -1;
			er.next_sibling				  = -1;
			er.visible					  = !em.get_entity_flags(h).is_set(entity_flags::entity_flags_invisible);

			const entity_family& fam = em.get_entity_family(h);

			if (!fam.parent.is_null())
			{
				auto it = index_by_world.find(fam.parent.index);
				if (it != index_by_world.end())
					er.parent = it->second;
			}
			if (!fam.first_child.is_null())
			{
				auto it = index_by_world.find(fam.first_child.index);
				if (it != index_by_world.end())
					er.first_child = it->second;
			}
			if (!fam.next_sibling.is_null())
			{
				auto it = index_by_world.find(fam.next_sibling.index);
				if (it != index_by_world.end())
					er.next_sibling = it->second;
			}

			je.push_back(er);

			const entity_comp_register& reg = em.get_component_register(h);

			for (const entity_comp& c : reg.comps)
			{
				json comp_json = {};

				comp_json["comp_type"] = c.comp_type;
				comp_json["entity"]	   = i;

				void*		ptr	   = cm.get_component(c.comp_type, c.comp_handle);
				const auto& fields = reflection::get().resolve(c.comp_type).get_fields();

				for (field_base* f : fields)
				{
					const reflected_field_type ft = f->_type;

					if (ft == reflected_field_type::rf_float)
					{
						const float val				 = f->value(ptr).cast<float>();
						comp_json[f->_title.c_str()] = val;
					}
					else if (ft == reflected_field_type::rf_int)
					{
						const int32 val				 = f->value(ptr).cast<int32>();
						comp_json[f->_title.c_str()] = val;
					}
					else if (ft == reflected_field_type::rf_uint)
					{
						const uint32 val			 = f->value(ptr).cast<uint32>();
						comp_json[f->_title.c_str()] = val;
					}
					else if (ft == reflected_field_type::rf_bool || ft == reflected_field_type::rf_uint8 || ft == reflected_field_type::rf_enum)
					{
						const uint8 val				 = f->value(ptr).cast<uint8>();
						comp_json[f->_title.c_str()] = val;
					}
					else if (ft == reflected_field_type::rf_vector2)
					{
						const vector2 val			 = f->value(ptr).cast<vector2>();
						comp_json[f->_title.c_str()] = val;
					}
					else if (ft == reflected_field_type::rf_vector3)
					{
						const vector3 val			 = f->value(ptr).cast<vector3>();
						comp_json[f->_title.c_str()] = val;
					}
					else if (ft == reflected_field_type::rf_vector4)
					{
						const vector4 val			 = f->value(ptr).cast<vector4>();
						comp_json[f->_title.c_str()] = val;
					}
					else if (ft == reflected_field_type::rf_color)
					{
						const color val				 = f->value(ptr).cast<color>();
						comp_json[f->_title.c_str()] = val;
					}
					else if (ft == reflected_field_type::rf_string)
					{
						const string val			 = f->value(ptr).cast<string>();
						comp_json[f->_title.c_str()] = val;
					}
					else if (ft == reflected_field_type::rf_resource)
					{
						if (f->_is_list)
						{
							const vector<resource_handle>& v  = f->value(ptr).cast_ref<vector<resource_handle>>();
							const uint32				   sz = static_cast<uint32>(v.size());
							comp_json[f->_title.c_str()]	  = sz;

							for (uint32 i = 0; i < v.size(); i++)
							{
								const string t = string(f->_title.c_str()) + std::to_string(i);

								const resource_handle res_h = v[i];
								if (!res_h.is_null())
								{
									const string& p		 = rm.get_loaded_path_by_handle(f->_sub_type_id, res_h);
									comp_json[t.c_str()] = p;
								}
								else
									comp_json[t.c_str()] = "";
							}
						}
						else
						{
							const resource_handle res_h = f->value(ptr).cast<resource_handle>();
							if (!res_h.is_null())
							{
								const string& p				 = rm.get_loaded_path_by_handle(f->_sub_type_id, res_h);
								comp_json[f->_title.c_str()] = p;
							}
							else
								comp_json[f->_title.c_str()] = "";
						}
					}
					else if (ft == reflected_field_type::rf_entity)
					{
						if (f->_is_list)
						{

							const vector<world_handle>& v  = f->value(ptr).cast_ref<vector<world_handle>>();
							const uint32				sz = static_cast<uint32>(v.size());
							comp_json[f->_title.c_str()]   = sz;

							for (uint32 i = 0; i < sz; i++)
							{
								const string t		 = string(f->_title.c_str()) + std::to_string(i);
								comp_json[t.c_str()] = index_by_world[v[i].index];
							}
						}
						else
						{
							const world_handle w		 = f->value(ptr).cast<world_handle>();
							const int32		   index	 = index_by_world[w.index];
							comp_json[f->_title.c_str()] = index;
						}
					}
				}
				jc.push_back(comp_json);
			}
		}
	}

	void entity_template_raw::load_from_json(const nlohmann::json& json_data, entity_template_raw& r)
	{
		r.entities.clear();
		r.entities = json_data.value<vector<entity_template_entity_raw>>("entities", {});

		if (r.component_buffer.get_size() != 0)
			r.component_buffer.destroy();

		const json& comps = json_data["components"];
		if (comps.is_array())
		{
			for (const auto& c : comps)
			{
				const string_id comp_type	 = c.value<string_id>("comp_type", 0);
				const uint32	entity_index = c.value<uint32>("entity", 0);
				const auto&		fields		 = reflection::get().resolve(comp_type).get_fields();
				const uint32	fields_size	 = static_cast<uint32>(fields.size());
				r.component_buffer << comp_type;
				r.component_buffer << entity_index;
				r.component_buffer << fields_size;

				for (field_base* f : fields)
				{
					const reflected_field_type ft = f->_type;
					r.component_buffer << f->_sid;
					r.component_buffer << ft;

					if (ft == reflected_field_type::rf_float)
					{
						const float val = c.value<float>(f->_title.c_str(), 0.0f);
						r.component_buffer << val;
					}
					else if (ft == reflected_field_type::rf_int)
					{
						const int32 val = c.value<int32>(f->_title.c_str(), 0.0f);
						r.component_buffer << val;
					}
					else if (ft == reflected_field_type::rf_uint)
					{
						const uint32 val = c.value<uint32>(f->_title.c_str(), 0.0f);
						r.component_buffer << val;
					}
					else if (ft == reflected_field_type::rf_vector2)
					{
						const vector2 val = c.value<vector2>(f->_title.c_str(), vector2::zero);
						r.component_buffer << val;
					}
					else if (ft == reflected_field_type::rf_vector3)
					{
						const vector3 val = c.value<vector3>(f->_title.c_str(), vector3::zero);
						r.component_buffer << val;
					}
					else if (ft == reflected_field_type::rf_vector4)
					{
						const vector4 val = c.value<vector4>(f->_title.c_str(), vector4::zero);
						r.component_buffer << val;
					}
					else if (ft == reflected_field_type::rf_color)
					{
						const color val = c.value<color>(f->_title.c_str(), color::white);
						r.component_buffer << val;
					}
					else if (ft == reflected_field_type::rf_bool || ft == reflected_field_type::rf_uint8 || ft == reflected_field_type::rf_enum)
					{
						const uint8 val = c.value<uint8>(f->_title.c_str(), 0);
						r.component_buffer << val;
					}
					else if (ft == reflected_field_type::rf_string)
					{
						const string val = c.value<string>(f->_title.c_str(), "");
						r.component_buffer << val;
					}
					else if (ft == reflected_field_type::rf_resource)
					{
						r.component_buffer << f->_sub_type_id;

						if (f->_is_list)
						{
							const uint32 count = c.value<uint32>(f->_title.c_str(), 0);
							r.component_buffer << count;

							for (uint32 i = 0; i < count; i++)
							{
								const string t	 = std::string(f->_title.c_str()) + std::to_string(i);
								const string val = c.value<string>(t.c_str(), "");
								r.component_buffer << val;
							}
						}
						else
						{
							const string val   = c.value<string>(f->_title.c_str(), "");
							const uint32 count = 1;
							r.component_buffer << count;
							r.component_buffer << val;
						}
					}
					else if (ft == reflected_field_type::rf_entity)
					{
						if (f->_is_list)
						{
							const uint32 count = c.value<uint32>(f->_title.c_str(), 0);
							r.component_buffer << count;
							for (uint32 i = 0; i < count; i++)
							{
								const string t	 = std::string(f->_title.c_str()) + std::to_string(i);
								const int32	 val = c.value<int32>(t.c_str(), -1);
								r.component_buffer << val;
							}
						}
						else
						{
							const int32	 val   = c.value<int32>(f->_title.c_str(), -1);
							const uint32 count = 1;
							r.component_buffer << count;
							r.component_buffer << val;
						}
					}
				}
			}
		}
	}

	bool entity_template_raw::load_from_file(const char* relative_file, const char* base_path)
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

			name = relative_file;
			load_from_json(json_data, *this);
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading entity_template: {0}", e.what());
			return false;
		}

		SFG_INFO("Created entity_template from file: {0}", target_path);
		return true;
	}

	bool entity_template_raw::load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension)
	{
		const string sid_str		 = std::to_string(TO_SID(relative_path));
		const string relative		 = file_system::get_filename_from_path(relative_path);
		const string meta_cache_path = cache_folder_path + relative + "-" + sid_str + "_meta" + extension;
		const string data_cache_path = cache_folder_path + relative + "-" + sid_str + "_data" + extension;

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

	void entity_template_raw::save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const
	{
		const string sid_str			= std::to_string(TO_SID(name));
		const string relative			= file_system::get_filename_from_path(name);
		const string file_path			= resource_directory_path + name;
		const uint64 file_last_modified = file_system::get_last_modified_ticks(file_path);

		const string meta_cache_path = cache_folder_path + relative + "-" + sid_str + "_meta" + extension;
		const string data_cache_path = cache_folder_path + relative + "-" + sid_str + "_data" + extension;

		ostream out_stream;
		out_stream << file_path;
		out_stream << file_last_modified;
		serialization::save_to_file(meta_cache_path.c_str(), out_stream);

		out_stream.shrink(0);
		serialize(out_stream);
		serialization::save_to_file(data_cache_path.c_str(), out_stream);

		out_stream.destroy();
	}

	void entity_template_raw::get_sub_resources(vector<string>& out_res) const
	{
		for (const string& res : resources)
			out_res.push_back(res);
	}

#endif

}
