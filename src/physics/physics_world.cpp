#include "physics_world.hpp"
#include "io/log.hpp"
#include "io/assert.hpp"
#include "world/world.hpp"
#include "physics/physics_convert.hpp"
#include "resources/physical_material.hpp"
#include "world/traits/trait_physics.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <regex>
#include <stdarg.h>

#ifdef JPH_DEBUG_RENDERER
#include "physics_debug_renderer.hpp"
#endif

namespace
{
	// Callback for traces, connect this to your own trace function if you have one
	static void trace_impl(const char* inFMT, ...)
	{
		va_list args;
		va_start(args, inFMT);

		// Use vsnprintf to safely format the string into a buffer.
		char buffer[4096]; // Adjust buffer size as needed.
		vsnprintf(buffer, sizeof(buffer), inFMT, args);

		// Convert buffer to std::string for regex replacement.
		std::string formattedMessage(buffer);

		// Escape curly braces by replacing `{` with `{{` and `}` with `}}`.
		formattedMessage = std::regex_replace(formattedMessage, std::regex("\\{"), "{{");
		formattedMessage = std::regex_replace(formattedMessage, std::regex("\\}"), "}}");

		// Pass the formatted and sanitized message to LINA_ERR.
		SFG_INFO(formattedMessage.c_str());

		va_end(args);
	}

#ifdef JPH_ENABLE_ASSERTS

	// Callback for asserts, connect this to your own assert handler if you have one
	static bool assert_impl(const char* inExpression, const char* inMessage, const char* inFile, uint32 inLine)
	{
		SFG_ASSERT(false, inExpression);
		return true;
	};

#endif // JPH_ENABLE_ASSERTS

} // namespace

namespace SFG
{
	void physics_world::init()
	{
		JPH::RegisterDefaultAllocator();
		JPH::Trace = trace_impl;
		JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = assert_impl);
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		_system		= new JPH::PhysicsSystem();
		_allocator	= new JPH::TempAllocatorImpl(10 * 1024 * 1024);
		_job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1);

		set_gravity(vector3(0.0f, -9.81f, 0.0f));
		_added_bodies.reserve(MAX_ENTITIES);

		const uint32 cMaxBodies				= 1024;
		const uint32 cNumBodyMutexes		= 0;
		const uint32 cMaxBodyPairs			= 1024;
		const uint32 cMaxContactConstraints = 1024;
		_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, _bp_layer_interface, _object_bp_layer_filter, _layer_filter);
	}

	void physics_world::uninit()
	{
		delete _job_system;
		delete _allocator;
		delete _system;

		_job_system = nullptr;
		_allocator	= nullptr;
		_system		= nullptr;

		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}

	void physics_world::init_simulation()
	{
		
		static vector<JPH::BodyID> reuse_body_ids;
		reuse_body_ids.resize(0);

		trait_manager& tm = _game_world.get_trait_manager();
		tm.view<trait_physics>([this](trait_physics& trait) -> trait_view_result {
			JPH::Body* body = trait.get_body();

			if (body == nullptr)
			{
				trait.create_body(_game_world);
				body = trait.get_body();
			}

			bitmask<uint8>& flags = trait.get_flags();

			if (!flags.is_set(trait_physics::flags::trait_physics_flags_in_simulation))
			{
				flags.set(trait_physics::flags::trait_physics_flags_in_simulation);
				reuse_body_ids.push_back(body->GetID());
			}

			return trait_view_result::cont;
		});

		if (!reuse_body_ids.empty())
			add_bodies_to_world(reuse_body_ids.data(), static_cast<uint32>(reuse_body_ids.size()));
	

		_system->OptimizeBroadPhase();
	}

	void physics_world::uninit_simulation()
	{
		 JPH::BodyInterface& body_interface = _system->GetBodyInterface();
		 
		 static vector<JPH::BodyID> reuse_body_ids;
		 reuse_body_ids.resize(0);
		 for (uint32 id : _added_bodies)
		 	reuse_body_ids.push_back(JPH::BodyID(id));
		 body_interface.RemoveBodies(reuse_body_ids.data(), static_cast<int32>(reuse_body_ids.size()));
		 _added_bodies.resize(0);
	}

	void physics_world::simulate(float rate)
	{
		constexpr int collision_steps = 1;
		_system->Update(rate, collision_steps, _allocator, _job_system);

		entity_manager& em = _game_world.get_entity_manager();

		trait_manager& tm = _game_world.get_trait_manager();
		tm.view<trait_physics>([&](trait_physics& trait) -> trait_view_result {
			JPH::Body* body = trait.get_body();
			SFG_ASSERT(body != nullptr);

			const world_handle e_handle = trait.get_header().entity;
			em.set_entity_position_abs(e_handle, from_jph_vec3(body->GetPosition()));
			em.set_entity_rotation_abs(e_handle, from_jph_quat(body->GetRotation()));
			return trait_view_result::cont;
		});
	}

	void physics_world::add_bodies_to_world(JPH::BodyID* body_ids, uint32 count)
	{
		JPH::BodyInterface&				   body_interface = _system->GetBodyInterface();
		const JPH::BodyInterface::AddState add_state	  = body_interface.AddBodiesPrepare(body_ids, static_cast<int>(count));
		body_interface.AddBodiesFinalize(body_ids, static_cast<int>(count), add_state, JPH::EActivation::Activate);

		for (uint32 i = 0; i < count; i++)
			_added_bodies.push_back(body_ids[i].GetIndexAndSequenceNumber());
	}

	void physics_world::add_body_to_world(const JPH::Body& body)
	{
		JPH::BodyID jph_id = body.GetID();
		add_bodies_to_world(&jph_id, 1);
	}

	void physics_world::remove_body_from_world(const JPH::Body& body)
	{
		const uint32 body_id = body.GetID().GetIndexAndSequenceNumber();
		_added_bodies.erase(std::find_if(_added_bodies.begin(), _added_bodies.end(), [body_id](uint32 id) -> bool { return id == body_id; }));
		JPH::BodyInterface& body_interface = _system->GetBodyInterface();
		body_interface.RemoveBody(body.GetID());
	}

	void physics_world::remove_bodies_from_world(JPH::BodyID* body_ids, uint32 count)
	{
		JPH::BodyInterface& body_interface = _system->GetBodyInterface();
		body_interface.RemoveBodies(body_ids, static_cast<int>(count));
	}

	JPH::Body* physics_world::create_body(physics_body_type body_type, physics_shape_type shape, const vector3& extents_or_height_radius, resource_handle mat, const vector3& pos, const quat& rot, const vector3& scale)
	{
		resource_manager& rm = _game_world.get_resource_manager();

		physical_material_settings mat_settings = _default_material;

		if (!mat.is_null())
		{
			physical_material& phy_mat = rm.get_resource<physical_material>(mat);
			mat_settings			   = phy_mat.get_settings();
		}

		JPH::BodyInterface& body_interface = _system->GetBodyInterface();

		JPH::BodyCreationSettings body_settings = {};

		JPH::Ref<JPH::Shape> shape_ref;

		if (shape == physics_shape_type::box)
		{
			JPH::BoxShapeSettings boxShapeSettings(to_jph_vec3(extents_or_height_radius * scale));
			shape_ref = boxShapeSettings.Create().Get();
		}
		else if (shape == physics_shape_type::sphere)
		{
			JPH::SphereShapeSettings sphere(extents_or_height_radius.y);
			shape_ref = sphere.Create().Get();
		}
		else if (shape == physics_shape_type::capsule)
		{
			JPH::CapsuleShapeSettings capsuleShapeSettings(extents_or_height_radius.x * 0.5f, extents_or_height_radius.y);
			shape_ref = capsuleShapeSettings.Create().Get();
		}
		else if (shape == physics_shape_type::cylinder)
		{
			JPH::CylinderShapeSettings cylinderShapeSettings(extents_or_height_radius.x * 0.5f, extents_or_height_radius.y);
			shape_ref = cylinderShapeSettings.Create().Get();
		}
		else if (shape == physics_shape_type::plane)
		{
			JPH::PlaneShapeSettings plane(JPH::Plane(to_jph_vec3(rot.get_up()), 0.0f));
			shape_ref = plane.Create().Get();
		}

		body_settings.mRestitution					= mat_settings.restitution;
		body_settings.mFriction						= mat_settings.friction;
		body_settings.mAngularDamping				= mat_settings.angular_damp;
		body_settings.mLinearDamping				= mat_settings.linear_damp;
		body_settings.mOverrideMassProperties		= JPH::EOverrideMassProperties::CalculateInertia;
		body_settings.mMassPropertiesOverride.mMass = mat_settings.mass;
		body_settings.mGravityFactor				= mat_settings.gravity_multiplier;
		body_settings.mMotionType					= static_cast<JPH::EMotionType>(body_type);
		body_settings.mObjectLayer					= body_type == physics_body_type::static_body ? static_cast<uint16>(physics_object_layers::non_moving) : static_cast<uint16>(physics_object_layers::moving);
		body_settings.mPosition						= to_jph_vec3(pos);
		body_settings.mRotation						= to_jph_quat(rot);
		body_settings.SetShape(shape_ref);

		JPH::Body* b = nullptr;
		return body_interface.CreateBody(body_settings);
	}

	void physics_world::destroy_body(JPH::Body* body)
	{
		JPH::BodyInterface& body_interface = _system->GetBodyInterface();
		body_interface.DestroyBody(body->GetID());
	}

	void physics_world::set_gravity(const vector3& g)
	{
		_graivty = g;
		_system->SetGravity(to_jph_vec3(g));
	}
}