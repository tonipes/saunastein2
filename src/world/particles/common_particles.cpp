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

#include "common_particles.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{
	void particle_emit_properties::serialize(ostream& stream) const
	{
		stream << spawn;
		stream << pos;
		stream << velocity;
		stream << rotation;
		stream << size;
		stream << color;
	}

	void particle_emit_properties::deserialize(istream& stream)
	{
		stream >> spawn;
		stream >> pos;
		stream >> velocity;
		stream >> rotation;
		stream >> size;
		stream >> color;
	}

	void particle_spawn_settings::serialize(ostream& stream) const
	{
		stream << emitter_lifetime;
		stream << wait_between_emits;
		stream << min_particle_count;
		stream << max_particle_count;
		stream << min_lifetime;
		stream << max_lifetime;
	}

	void particle_spawn_settings::deserialize(istream& stream)
	{
		stream >> emitter_lifetime;
		stream >> wait_between_emits;
		stream >> min_particle_count;
		stream >> max_particle_count;
		stream >> min_lifetime;
		stream >> max_lifetime;
	}

	void particle_position_settings::serialize(ostream& stream) const
	{
		stream << min_start;
		stream << max_start;
		stream << cone_radius;
	}

	void particle_position_settings::deserialize(istream& stream)
	{
		stream >> min_start;
		stream >> max_start;
		stream >> cone_radius;
	}

	void particle_velocity_settings::serialize(ostream& stream) const
	{
		stream << min_start;
		stream << max_start;
		stream << min_mid;
		stream << max_mid;
		stream << min_end;
		stream << max_end;
		stream << integrate_point;
		stream << is_local;
	}

	void particle_velocity_settings::deserialize(istream& stream)
	{
		stream >> min_start;
		stream >> max_start;
		stream >> min_mid;
		stream >> max_mid;
		stream >> min_end;
		stream >> max_end;
		stream >> integrate_point;
		stream >> is_local;
	}
	void particle_color_settings::serialize(ostream& stream) const
	{
		stream << min_start;
		stream << max_start;
		stream << min_start_opacity;
		stream << max_start_opacity;
		stream << mid_opacity;
		stream << end_opacity;
		stream << integrate_point_opacity;
	}
	void particle_color_settings::deserialize(istream& stream)
	{
		stream >> min_start;
		stream >> max_start;
		stream >> min_start_opacity;
		stream >> max_start_opacity;
		stream >> mid_opacity;
		stream >> end_opacity;
		stream >> integrate_point_opacity;
	}
	void particle_rotation_settings::serialize(ostream& stream) const
	{
		stream << min_start_rotation;
		stream << max_start_rotation;
		stream << min_start_angular_velocity;
		stream << max_start_angular_velocity;
		stream << min_end_angular_velocity;
		stream << max_end_angular_velocity;
		stream << integrate_point_angular_velocity;
	}
	void particle_rotation_settings::deserialize(istream& stream)
	{
		stream >> min_start_rotation;
		stream >> max_start_rotation;
		stream >> min_start_angular_velocity;
		stream >> max_start_angular_velocity;
		stream >> min_end_angular_velocity;
		stream >> max_end_angular_velocity;
		stream >> integrate_point_angular_velocity;
	}
	void particle_size_settings::serialize(ostream& stream) const
	{
		stream << min_start;
		stream << max_start;
		stream << mid;
		stream << end;
		stream << integrate_point;
	}
	void particle_size_settings::deserialize(istream& stream)
	{
		stream >> min_start;
		stream >> max_start;
		stream >> mid;
		stream >> end;
		stream >> integrate_point;
	}
}