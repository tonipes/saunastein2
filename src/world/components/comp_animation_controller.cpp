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

#include "comp_animation_controller.hpp"
#include "reflection/type_reflection.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "world/world.hpp"
#include "reflection/reflection.hpp"
#include "resources/anim_state_machine.hpp"
#include "resources/animation.hpp"
namespace SFG
{
	void comp_animation_controller::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_animation_controller>::value, 0, "component");
		m.set_title("anim_controller");
		m.add_field<&comp_animation_controller::_resource_machine, comp_animation_controller>(
			"machine", reflected_field_type::rf_resource, "", type_id<anim_state_machine>::value);

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_animation_controller* c = static_cast<comp_animation_controller*>(params.object_ptr);
			c->set_machine_resource(params.w, c->_resource_machine);
		});
	}

	void comp_animation_controller::on_add(world& w)
	{
	}

	void comp_animation_controller::on_remove(world& w)
	{
	}

	void comp_animation_controller::serialize(ostream& stream, world& w) const
	{
	}

	void comp_animation_controller::deserialize(istream& stream, world& w)
	{
	}

	void comp_animation_controller::set_machine_resource(world& w, resource_handle h)
	{
		_resource_machine = h;

		animation_graph& anim_graph = w.get_animation_graph();

		if (!_runtime_machine.is_null())
		{
			anim_graph.remove_state_machine(_runtime_machine);
			_runtime_machine = {};
		}

		resource_manager&			  rm  = w.get_resource_manager();
		anim_state_machine&			  as  = rm.get_resource<anim_state_machine>(_resource_machine);
		const anim_state_machine_raw& raw = as.get_raw();

		animation_graph&  ag = w.get_animation_graph();

		_runtime_machine									  = ag.add_state_machine();
		anim_graph.get_state_machine(_runtime_machine).entity = _header.entity;

		struct pair_str_param
		{
			string		  name;
			pool_handle16 handle;
		};
		vector<pair_str_param> param_map;
		param_map.reserve(raw.parameters.size());

		for (const auto& p : raw.parameters)
		{
			const pool_handle16 ph = ag.add_parameter(_runtime_machine, p.value);
			param_map.push_back({p.name, ph});
		}

		auto find_param = [&param_map](const string& name) -> pool_handle16 {
			if (name.empty())
				return pool_handle16();
			for (const auto& pr : param_map)
				if (pr.name == name)
					return pr.handle;
			return {};
		};

		struct pair_str_state
		{
			string		  name;
			pool_handle16 handle;
		};
		vector<pair_str_state> state_map;
		state_map.reserve(raw.states.size());

		pool_handle16 default_initial = {};

		for (const auto& s : raw.states)
		{
			uint8 flags = 0;
			if (s.is_looping)
				flags |= animation_state_flags_is_looping;
			if (s.blend_type == 1)
				flags |= animation_state_flags_is_1d;
			else if (s.blend_type == 2)
				flags |= animation_state_flags_is_2d;

			const pool_handle16 px = find_param(s.blend_param_x);
			const pool_handle16 py = find_param(s.blend_param_y);

			const pool_handle16 st = ag.add_state(_runtime_machine, {}, px, py, s.duration, flags);

			// set speed
			animation_state& stref = ag.get_state(st);
			stref.speed			   = s.speed;

			for (const auto& smp : s.samples)
			{
				const resource_handle ah = rm.get_resource_handle_by_hash_if_exists<animation>(smp.animation_sid);
				if (!ah.is_null() && rm.is_valid<animation>(ah))
					ag.add_state_sample(st, ah, smp.blend_point);
			}

			if (s.duration <= 0.0f && !s.samples.empty())
			{
				const resource_handle ah0 = rm.get_resource_handle_by_hash_if_exists<animation>(s.samples[0].animation_sid);
				if (!ah0.is_null())
					stref.duration = rm.get_resource<animation>(ah0).get_duration();
			}

			if (default_initial.is_null())
				default_initial = st;
			state_map.push_back({s.name, st});
		}

		auto find_state = [&state_map](const string& name) -> pool_handle16 {
			if (name.empty())
				return pool_handle16();
			for (const auto& st : state_map)
				if (st.name == name)
					return st.handle;
			return {};
		};

		for (const auto& t : raw.transitions)
		{
			const pool_handle16 from = find_state(t.from_state);
			const pool_handle16 to	 = find_state(t.to_state);
			const pool_handle16 pr	 = find_param(t.parameter);
			if (!from.is_null() && !to.is_null())
			{
				ag.add_transition(from, to, pr, t.duration, t.target, t.compare, t.priority);
			}
		}

		pool_handle16 initial = find_state(raw.initial_state);
		if (initial.is_null())
			initial = default_initial;
		if (!initial.is_null())
			ag.set_machine_active_state(_runtime_machine, initial);
	}

}
