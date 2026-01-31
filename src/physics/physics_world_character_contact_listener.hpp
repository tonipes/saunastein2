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

#include "world/world_constants.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace JPH
{
	class CharacterVirtual;
	class BodyID;
	class SubShapeID;
	class CharacterContactSettings;
}

namespace SFG
{
	class physics_world;
	class physics_character_contact_listener;
	struct vector3;

	class physics_world_character_contact_listener final : public JPH::CharacterContactListener
	{
	public:
		explicit physics_world_character_contact_listener(physics_world& world);

		void set_listener(physics_character_contact_listener* listener);

		void OnContactAdded(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings) override;
		void OnContactPersisted(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings) override;
		void OnContactRemoved(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2) override;

	private:
		static world_handle unpack_world_handle(uint64 packed);
		static vector3		to_vector3(const JPH::RVec3& v);

		void dispatch_contact_begin(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal);
		void dispatch_contact(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal);

		physics_world&						_world;
		physics_character_contact_listener* _listener = nullptr;
	};
}
