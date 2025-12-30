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
#include "common/size_definitions.hpp"
#include "math/vector3.hpp"
#include "math/vector2.hpp"
#include "math/color.hpp"

namespace SFG
{
	class ostream;
	class istream;

	struct particle_emit_properties
	{
		float	emitter_lifetime		 = 0.0f;
		float	wait_between_emits		 = 0.0f;
		uint32	min_particle_count		 = 1;
		uint32	max_particle_count		 = 1;
		vector3 min_pos_offset			 = vector3::zero;
		vector3 max_pos_offset			 = vector3::zero;
		vector3 min_vel_offset			 = vector3::zero;
		vector3 max_vel_offset			 = vector3::zero;
		color	min_color				 = color::white;
		color	max_color				 = color::white;
		vector2 min_max_opacity_velocity = vector2::zero;
		vector2 min_max_rotation_deg	 = vector2::zero;
		vector2 min_max_angular_velocity = vector2::zero;
		vector2 min_max_lifetime		 = vector2::one;
		vector2 min_max_size			 = vector2::one;
		vector2 min_max_size_velocity	 = vector2::zero;


		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};
}
