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

#include "physics/physics_world_contact_listener.hpp"
#include "physics/physics_world.hpp"
#include "physics/physics_contact_listener.hpp"
#include "math/vector3.hpp"
#include <Jolt/Physics/Body/Body.h>

namespace SFG
{
	physics_world_contact_listener::physics_world_contact_listener(physics_world& world) : _world(world)
	{
	}

	void physics_world_contact_listener::set_listener(physics_contact_listener* listener)
	{
		_listener = listener;
	}

	void physics_world_contact_listener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		(void)ioSettings;
		dispatch_contact_begin(inBody1, inBody2, inManifold);
	}

	void physics_world_contact_listener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		(void)ioSettings;
		dispatch_contact(inBody1, inBody2, inManifold);
	}

	void physics_world_contact_listener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
	{
		if (_listener == nullptr)
			return;

		world_handle e1 = _world.get_comp_physics_entity_by_id(inSubShapePair.GetBody1ID());
		world_handle e2 = _world.get_comp_physics_entity_by_id(inSubShapePair.GetBody2ID());
		if (e1.is_null() || e2.is_null())
			return;

		_listener->on_contact_end(e1, e2);
	}

	vector3 physics_world_contact_listener::to_vector3(const JPH::RVec3& v)
	{
		return vector3(static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ()));
	}

	void physics_world_contact_listener::dispatch_contact_begin(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold)
	{
		if (_listener == nullptr)
			return;

		world_handle e1 = _world.get_comp_physics_entity_by_id(inBody1.GetID());
		world_handle e2 = _world.get_comp_physics_entity_by_id(inBody2.GetID());
		if (e1.is_null() || e2.is_null())
			return;

		const vector3 p1 = to_vector3(inManifold.GetWorldSpaceContactPointOn1(0));
		const vector3 p2 = to_vector3(inManifold.GetWorldSpaceContactPointOn2(0));
		_listener->on_contact_begin(e1, e2, p1, p2);
	}

	void physics_world_contact_listener::dispatch_contact(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold)
	{
		if (_listener == nullptr)
			return;

		world_handle e1 = _world.get_comp_physics_entity_by_id(inBody1.GetID());
		world_handle e2 = _world.get_comp_physics_entity_by_id(inBody2.GetID());
		if (e1.is_null() || e2.is_null())
			return;

		const vector3 p1 = to_vector3(inManifold.GetWorldSpaceContactPointOn1(0));
		const vector3 p2 = to_vector3(inManifold.GetWorldSpaceContactPointOn2(0));
		_listener->on_contact(e1, e2, p1, p2);
	}
}
