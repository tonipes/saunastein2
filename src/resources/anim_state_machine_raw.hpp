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

#pragma once

#include "data/vector.hpp"
#include "data/string.hpp"
#include "math/vector2.hpp"
#include "common/string_id.hpp"
#include "world/animation/animation_transition.hpp"

namespace SFG
{
	class ostream;
	class istream;

	struct anim_sm_parameter_raw
	{
		string name	 = "";
		float  value = 0.0f;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct anim_sm_state_sample_raw
	{
		string_id animation_sid = 0;
		vector2	  blend_point	= vector2::zero;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct anim_sm_state_raw
	{
		string							 name		   = "";
		float							 duration	   = 0.0f;
		float							 speed		   = 1.0f;
		uint8							 is_looping	   = 1;
		uint8							 blend_type	   = 1;
		string							 blend_param_x = "";
		string							 blend_param_y = "";
		vector<anim_sm_state_sample_raw> samples;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct anim_sm_transition_raw
	{
		string						 from_state = "";
		string						 to_state	= "";
		string						 parameter	= "";
		float						 duration	= 0.0f;
		float						 target		= 0.0f;
		uint8						 priority	= 0;
		animation_transition_compare compare	= animation_transition_compare::greater;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct anim_state_machine_raw
	{
		string						   name			 = "";
		string						   initial_state = "";
		vector<anim_sm_parameter_raw>  parameters;
		vector<anim_sm_state_raw>	   states;
		vector<anim_sm_transition_raw> transitions;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

#ifdef SFG_TOOLMODE
		bool load_from_file(const char* relative_file, const char* base_path);
		bool load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension);
		void save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const;
		void get_dependencies(vector<string>& out_deps) const {};
#endif
	};
}
