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

#include "physics/physics_world_character_contact_listener.hpp"
#include "physics/physics_world.hpp"
#include "physics/physics_character_contact_listener.hpp"
#include "physics/physics_convert.hpp"
#include "math/vector3.hpp"

namespace SFG
{
	physics_world_character_contact_listener::physics_world_character_contact_listener(physics_world& world) : _world(world)
	{
	}

	void physics_world_character_contact_listener::set_listener(physics_character_contact_listener* listener)
	{
		_listener = listener;
	}

	void physics_world_character_contact_listener::OnContactAdded(const JPH::CharacterVirtual* inCharacter,
																  const JPH::BodyID& inBodyID2,
																  const JPH::SubShapeID& inSubShapeID2,
																  JPH::RVec3Arg inContactPosition,
																  JPH::Vec3Arg inContactNormal,
																  JPH::CharacterContactSettings& ioSettings)
	{
		(void)inSubShapeID2;
		(void)ioSettings;
		dispatch_contact_begin(inCharacter, inBodyID2, inContactPosition, inContactNormal);
	}

	void physics_world_character_contact_listener::OnContactPersisted(const JPH::CharacterVirtual* inCharacter,
																	  const JPH::BodyID& inBodyID2,
																	  const JPH::SubShapeID& inSubShapeID2,
																	  JPH::RVec3Arg inContactPosition,
																	  JPH::Vec3Arg inContactNormal,
																	  JPH::CharacterContactSettings& ioSettings)
	{
		(void)inSubShapeID2;
		(void)ioSettings;
		dispatch_contact(inCharacter, inBodyID2, inContactPosition, inContactNormal);
	}

	void physics_world_character_contact_listener::OnContactRemoved(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2)
	{
		(void)inSubShapeID2;
		if (_listener == nullptr || inCharacter == nullptr)
			return;

		world_handle character = unpack_world_handle(inCharacter->GetUserData());
		if (character.is_null())
			return;

		world_handle other = _world.get_comp_physics_entity_by_id(inBodyID2);
		if (other.is_null())
			return;

		_listener->on_character_contact_end(character, other);
	}

	world_handle physics_world_character_contact_listener::unpack_world_handle(uint64 packed)
	{
		world_handle h = {};
		h.generation   = static_cast<uint32>(packed >> 32);
		h.index		   = static_cast<uint32>(packed & 0xffffffff);
		return h;
	}

	vector3 physics_world_character_contact_listener::to_vector3(const JPH::RVec3& v)
	{
		return vector3(static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ()));
	}

	void physics_world_character_contact_listener::dispatch_contact_begin(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal)
	{
		if (_listener == nullptr || inCharacter == nullptr)
			return;

		world_handle character = unpack_world_handle(inCharacter->GetUserData());
		if (character.is_null())
			return;

		world_handle other = _world.get_comp_physics_entity_by_id(inBodyID2);
		if (other.is_null())
			return;

		const vector3 pos	 = to_vector3(inContactPosition);
		const vector3 normal = from_jph_vec3(inContactNormal);
		_listener->on_character_contact_begin(character, other, pos, normal);
	}

	void physics_world_character_contact_listener::dispatch_contact(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal)
	{
		if (_listener == nullptr || inCharacter == nullptr)
			return;

		world_handle character = unpack_world_handle(inCharacter->GetUserData());
		if (character.is_null())
			return;

		world_handle other = _world.get_comp_physics_entity_by_id(inBodyID2);
		if (other.is_null())
			return;

		const vector3 pos	 = to_vector3(inContactPosition);
		const vector3 normal = from_jph_vec3(inContactNormal);
		_listener->on_character_contact(character, other, pos, normal);
	}
}
