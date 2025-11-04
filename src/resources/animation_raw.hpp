// Copyright (c) 2025 Inan Evin
#pragma once

#include "animation_common.hpp"
#include "data/vector.hpp"
#include "data/string.hpp"
#include "common/string_id.hpp"

namespace SFG
{
	class ostream;
	class istream;

	struct animation_channel_v3_raw
	{
		animation_interpolation				 interpolation = animation_interpolation::linear;
		vector<animation_keyframe_v3>		 keyframes;
		vector<animation_keyframe_v3_spline> keyframes_spline;
		int16								 node_index = -1;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct animation_channel_q_raw
	{
		animation_interpolation				interpolation = animation_interpolation::linear;
		vector<animation_keyframe_q>		keyframes;
		vector<animation_keyframe_q_spline> keyframes_spline;
		int16								node_index = -1;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct animation_raw
	{
		string							 name = "";
		vector<animation_channel_v3_raw> position_channels;
		vector<animation_channel_q_raw>	 rotation_channels;
		vector<animation_channel_v3_raw> scale_channels;
		string_id						 sid	  = 0;
		float							 duration = 0.0f;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
		void save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const
		{
		}
		bool load_from_file(const char* relative_file, const char* base_path)
		{
			return false;
		};

		bool load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension)
		{
			return false;
		}

		void get_dependencies(vector<string>& out_deps) const {};
	};

}
