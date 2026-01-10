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

#include "anim_state_machine_raw.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

#ifdef SFG_TOOLMODE
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "serialization/serialization.hpp"
#include <vendor/nhlohmann/json.hpp>
#include <fstream>
using json = nlohmann::json;
#endif

namespace SFG
{
	void anim_sm_parameter_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << value;
	}

	void anim_sm_parameter_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> value;
	}

	void anim_sm_state_sample_raw::serialize(ostream& stream) const
	{
		stream << animation_sid;
		stream << base_model_str;
		stream << blend_point;
	}

	void anim_sm_state_sample_raw::deserialize(istream& stream)
	{
		stream >> animation_sid;
		stream >> base_model_str;
		stream >> blend_point;
	}

	void anim_sm_state_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << duration;
		stream << speed;
		stream << is_looping;
		stream << blend_type;
		stream << blend_param_x;
		stream << blend_param_y;
		stream << samples;
	}

	void anim_sm_state_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> duration;
		stream >> speed;
		stream >> is_looping;
		stream >> blend_type;
		stream >> blend_param_x;
		stream >> blend_param_y;
		stream >> samples;
	}

	void anim_sm_transition_raw::serialize(ostream& stream) const
	{
		stream << from_state;
		stream << to_state;
		stream << parameter;
		stream << duration;
		stream << target;
		stream << priority;
		stream << compare;
	}

	void anim_sm_transition_raw::deserialize(istream& stream)
	{
		stream >> from_state;
		stream >> to_state;
		stream >> parameter;
		stream >> duration;
		stream >> target;
		stream >> priority;
		stream >> compare;
	}

	void anim_state_machine_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << initial_state;
		stream << parameters;
		stream << states;
		stream << transitions;
	}

	void anim_state_machine_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> initial_state;
		stream >> parameters;
		stream >> states;
		stream >> transitions;
	}

#ifdef SFG_TOOLMODE

	static animation_transition_compare str_to_compare(const string& s)
	{
		if (s == "equals")
			return animation_transition_compare::equals;
		if (s == "not_equals")
			return animation_transition_compare::not_equals;
		if (s == "greater")
			return animation_transition_compare::greater;
		if (s == "lesser")
			return animation_transition_compare::lesser;
		if (s == "gequals")
			return animation_transition_compare::gequals;
		if (s == "lequals")
			return animation_transition_compare::lequals;
		return animation_transition_compare::greater;
	}

	bool anim_state_machine_raw::load_from_file(const char* relative_file, const char* base_path)
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
			json		  j = json::parse(f);
			f.close();

			name		  = relative_file;
			initial_state = j.value<string>("initial_state", "");

			// parameters
			if (j.contains("parameters"))
			{
				const auto& jp = j.at("parameters");
				for (const auto& p : jp)
				{
					anim_sm_parameter_raw pr = {};
					pr.name					 = p.value<string>("name", "");
					pr.value				 = p.value<float>("value", 0.0f);
					parameters.push_back(pr);
				}
			}

			// states
			if (j.contains("states"))
			{
				const auto& js = j.at("states");
				for (const auto& s : js)
				{
					anim_sm_state_raw st = {};
					st.name				 = s.value<string>("name", "");
					st.duration			 = s.value<float>("duration", 0.0f);
					st.speed			 = s.value<float>("speed", 1.0f);
					st.is_looping		 = s.value<uint8>("loop", 1);

					const string blend = s.value<string>("blend", "1d");
					if (blend == "2d")
						st.blend_type = 2;
					else if (blend == "none")
						st.blend_type = 0;
					else
						st.blend_type = 1;

					st.blend_param_x = s.value<string>("blend_param_x", "");
					st.blend_param_y = s.value<string>("blend_param_y", "");

					if (s.contains("samples"))
					{
						const auto& jsmps = s.at("samples");
						for (const auto& sm : jsmps)
						{
							anim_sm_state_sample_raw ss		 = {};
							const string			 anim_id = sm.value<string>("animation", "");
							ss.animation_sid				 = TO_SID(anim_id);
							ss.blend_point					 = sm.value<vector2>("point", vector2::zero);

							const size_t last_s = anim_id.find_last_of("/");
							if (last_s != string::npos)
								ss.base_model_str = anim_id.substr(0, last_s);

							st.samples.push_back(ss);
						}
					}

					states.push_back(st);
				}
			}

			// transitions
			if (j.contains("transitions"))
			{
				const auto& jt = j.at("transitions");
				for (const auto& t : jt)
				{
					anim_sm_transition_raw tr = {};
					tr.from_state			  = t.value<string>("from", "");
					tr.to_state				  = t.value<string>("to", "");
					tr.parameter			  = t.value<string>("parameter", "");
					tr.duration				  = t.value<float>("duration", 0.0f);
					tr.target				  = t.value<float>("value", 0.0f);
					tr.priority				  = t.value<uint8>("priority", 0);
					tr.compare				  = str_to_compare(t.value<string>("compare", "greater"));
					transitions.push_back(tr);
				}
			}
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading anim_state_machine: {0}", e.what());
			return false;
		}

		SFG_INFO("Created anim_state_machine from file: {0}", target_path.c_str());
		return true;
	}

	bool anim_state_machine_raw::load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension)
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

	void anim_state_machine_raw::save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const
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

	void anim_state_machine_raw::get_sub_resources(vector<string>& out_res) const
	{
		for (const anim_sm_state_raw& sr : states)
		{
			for (const anim_sm_state_sample_raw& r : sr.samples)
				out_res.push_back(r.base_model_str);
		}
	}

#endif
}
