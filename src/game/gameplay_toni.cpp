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

#include "gameplay.hpp"
#include "app/app.hpp"
#include "world/world.hpp"
#include "game/components/comp_player.hpp"
#include "game/components/comp_player_stats.hpp"
#include <world/components/comp_physics.hpp>
#include "game/components/comp_enemy_ai_basic.hpp"
#include <physics/physics_contact_listener.hpp>
#include "platform/window_common.hpp"
#include "input/input_mappings.hpp"
#include "resources/entity_template.hpp"
#include "math/vector2.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <world/components/comp_camera.hpp>

namespace SFG
{
	namespace
	{
		static const char* k_cutscene_subtitles[] = {
			"",
			"The steam hangs in the air.",
			"You feel the heat rising.",
			"Something stirs beyond the door.",
			"I feel like moisturizing",
		};

		static constexpr int k_cutscene_subtitle_count = static_cast<int>(sizeof(k_cutscene_subtitles) / sizeof(k_cutscene_subtitles[0]));
	}

	void gameplay::tick_doors(float dt)
	{
		world&			   w  = _app.get_world();
		component_manager& cm = w.get_comp_manager();
		entity_manager&	   em = w.get_entity_manager();
		physics_world&	   ph = w.get_physics_world();

		float t = w.get_time_manager().get_elapsed_game_time();

		// doors
		for (int i = 0; i < _doors.size(); ++i)
		{
			if (_doors[i].door_root_handle.is_null())
				continue;

			if (!_doors[i].is_opened)
			{
				vector3 player_pos = em.get_entity_position_abs(_player_entity);
				vector3 door_pos   = em.get_entity_position_abs(_doors[i].door_root_handle);
				quat	door_rot   = em.get_entity_rotation_abs(_doors[i].door_root_handle);

				float distance = (player_pos - door_pos).magnitude();
				if (distance < _doors[i].auto_open_distance)
				{
					_doors[i].is_opened			 = true;
					vector3 door_forward		 = door_rot.get_forward();
					vector3 to_player			 = (player_pos - door_pos).normalized();
					float	dot					 = vector3::dot(door_forward, to_player);
					bool	should_open_backward = dot > 0.0f;
					if (should_open_backward)
					{
						_doors[i].direction *= -1.0f;
					}
				}
			}

			if (!_doors[i].is_opened)
				continue;

			float speed = 1.0f;
			_doors[i].t += dt * speed;
			float tt = _doors[i].t;
			if (tt > 1.0f)
				tt = 1.0f;
			if (tt < 0.0f)
				tt = 0.0f;

			em.visit_children(_doors[i].door_root_handle, [&](world_handle child) {
				float ss = _doors[i].direction * -1.0f;
				if (em.get_entity_scale(child).x < 0.0f)
					ss *= -1.0f;

				const quat rot = quat::from_euler(0.0f, ss * tt * _doors[i].open_angle, 0.0f);
				em.set_entity_rotation(child, rot);

				world_handle  phys_ent_handle  = em.get_child_by_index(child, 1);
				world_handle  phys_comp_handle = em.get_entity_component<comp_physics>(phys_ent_handle);
				comp_physics& phys_comp		   = cm.get_component<comp_physics>(phys_comp_handle);
				phys_comp.set_body_position_and_rotation(w, em.get_entity_position_abs(phys_ent_handle), em.get_entity_rotation_abs(phys_ent_handle));
			});
		}

		// pickups
		for (int i = _pickups.size() - 1; i >= 0; --i)
		{
			pickup& p = _pickups[i];

			if (!em.is_valid(p.root_handle))
			{
				_pickups.pop_back();
				continue;
			}

			vector3 player_pos = em.get_entity_position_abs(_player_entity);
			vector3 pickup_pos = em.get_entity_position_abs(_pickups[i].root_handle);
			float	dist	   = vector2::distance({player_pos.x, player_pos.z}, {pickup_pos.x, pickup_pos.z});

			if (!p.visual.is_null() && em.is_valid(p.visual))
			{
				em.set_entity_rotation(p.visual, quat::from_euler(0.0f, t * 200.0f, 0.0f));
				em.set_entity_position(p.visual, {0.0f, sinf(t * 2), 0.0f});
			}

			if (!em.is_valid(p.root_handle) || dist < 1.0f)
			{
				em.destroy_entity(p.root_handle);
				_pickups.pop_back();
			}
		}

		if (!_cutscene_camera.is_null() && !_cutscene_camera_waypoints.empty())
		{
			auto parse_waypoint_params = [&](world_handle wp, float& wait_seconds, bool& skip_segment, bool& has_subtitle, char* subtitle_out, size_t subtitle_cap) {
				wait_seconds = 0.0f;
				skip_segment = false;
				has_subtitle = false;
				if (subtitle_out != nullptr && subtitle_cap > 0)
					subtitle_out[0] = '\0';

				const char* name = em.get_entity_meta(wp).name;
				if (name == nullptr)
					return;

				const char* wait_ptr = std::strstr(name, "wait=");
				if (wait_ptr != nullptr)
				{
					char*		 end_ptr  = nullptr;
					const double wait_val = std::strtod(wait_ptr + 5, &end_ptr);
					if (end_ptr != (wait_ptr + 5) && wait_val > 0.0)
						wait_seconds = static_cast<float>(wait_val);
				}

				if (std::strstr(name, "skip") != nullptr)
					skip_segment = true;

				const char* subtitle_ptr = std::strstr(name, "st=");
				if (subtitle_ptr != nullptr && subtitle_out != nullptr && subtitle_cap > 0)
				{
					subtitle_ptr += 3;
					char*	   end_ptr = nullptr;
					const long idx_val = std::strtol(subtitle_ptr, &end_ptr, 10);
					if (end_ptr != subtitle_ptr && idx_val >= 0 && idx_val < k_cutscene_subtitle_count)
					{
						const char* src = k_cutscene_subtitles[idx_val];
						if (src != nullptr)
						{
							std::strncpy(subtitle_out, src, subtitle_cap - 1);
							subtitle_out[subtitle_cap - 1] = '\0';
							if (subtitle_out[0] != '\0')
								has_subtitle = true;
						}
					}
				}
			};

			if (_cutscene_camera_waypoint_index < 0)
				_cutscene_camera_waypoint_index = 0;

			const int last_index = _cutscene_camera_waypoints.size() - 1;

			if (_cutscene_camera_waypoint_index >= last_index)
			{
				const vector3 last_pos = em.get_entity_position_abs(_cutscene_camera_waypoints[last_index]);
				const quat	  last_rot = em.get_entity_rotation_abs(_cutscene_camera_waypoints[last_index]);
				em.set_entity_position_abs(_cutscene_camera, last_pos);
				em.set_entity_rotation_abs(_cutscene_camera, last_rot);
			}
			else
			{
				const world_handle current_wp = _cutscene_camera_waypoints[_cutscene_camera_waypoint_index];
				const world_handle next_wp	  = _cutscene_camera_waypoints[_cutscene_camera_waypoint_index + 1];
				const vector3	   start_pos  = em.get_entity_position_abs(current_wp);
				const vector3	   end_pos	  = em.get_entity_position_abs(next_wp);
				const quat		   start_rot  = em.get_entity_rotation_abs(current_wp);
				const quat		   end_rot	  = em.get_entity_rotation_abs(next_wp);

				if (_cutscene_camera_waypoint_index_last != _cutscene_camera_waypoint_index)
				{
					bool has_subtitle	   = false;
					char subtitle_buf[256] = {};
					parse_waypoint_params(current_wp, _cutscene_camera_wait_remaining, _cutscene_camera_skip_segment, has_subtitle, subtitle_buf, sizeof(subtitle_buf));
					set_cutscene_subtitle_text(has_subtitle ? subtitle_buf : "");
					_cutscene_camera_waypoint_index_last = _cutscene_camera_waypoint_index;
				}

				if (_cutscene_camera_wait_remaining > 0.0f)
				{
					_cutscene_camera_wait_remaining -= dt;
					if (_cutscene_camera_wait_remaining < 0.0f)
						_cutscene_camera_wait_remaining = 0.0f;

					em.set_entity_position_abs(_cutscene_camera, start_pos);
					em.set_entity_rotation_abs(_cutscene_camera, start_rot);
					return;
				}

				if (_cutscene_camera_skip_segment)
				{
					em.set_entity_position_abs(_cutscene_camera, end_pos);
					em.set_entity_rotation_abs(_cutscene_camera, end_rot);
					_cutscene_camera_waypoint_index++;
					_cutscene_camera_waypoint_t			 = 0.0f;
					_cutscene_camera_waypoint_index_last = -1;
					_cutscene_camera_skip_segment		 = false;
					return;
				}

				const float dist = vector3::distance(start_pos, end_pos);

				if (dist <= 0.001f)
				{
					_cutscene_camera_waypoint_index++;
					_cutscene_camera_waypoint_t = 0.0f;
					em.set_entity_position_abs(_cutscene_camera, end_pos);
					em.set_entity_rotation_abs(_cutscene_camera, end_rot);
				}
				else
				{
					_cutscene_camera_waypoint_t += (dt * _cutscene_camera_speed) / dist;
					if (_cutscene_camera_waypoint_t >= 1.0f)
					{
						_cutscene_camera_waypoint_t = 0.0f;
						_cutscene_camera_waypoint_index++;
						em.set_entity_position_abs(_cutscene_camera, end_pos);
						em.set_entity_rotation_abs(_cutscene_camera, end_rot);
					}
					else
					{
						const float	  t_smoothed = _cutscene_camera_waypoint_t * _cutscene_camera_waypoint_t * (3.0f - 2.0f * _cutscene_camera_waypoint_t);
						const vector3 pos		 = vector3::lerp(start_pos, end_pos, t_smoothed);
						const quat	  rot		 = quat::slerp(start_rot, end_rot, t_smoothed);
						em.set_entity_position_abs(_cutscene_camera, pos);
						em.set_entity_rotation_abs(_cutscene_camera, rot);
					}
				}
			}
		}
	}

	void gameplay::begin_doors()
	{
		_doors.clear();
		_pickups.clear();

		world&			   w  = _app.get_world();
		component_manager& cm = w.get_comp_manager();
		entity_manager&	   em = w.get_entity_manager();
		physics_world&	   ph = w.get_physics_world();

		vector<world_handle> tmp = {};

		tmp.clear();
		em.find_entities_by_tag("door_root", tmp);
		for (int i = 0; i < tmp.size(); ++i)
		{
			// SFG_TRACE("DOOR: {0}", i);
			auto name	 = em.get_entity_meta(tmp[i]).name;
			bool is_auto = strstr(name, "noauto") == NULL;

			door d = {
				.door_root_handle	= tmp[i],
				.t					= 0,
				.open_angle			= 165.0f,
				.is_opened			= false,
				.auto_open_distance = is_auto ? 10.0f : 0.0f,
				.direction			= 1.0f,
			};

			_doors.push_back(d);
		}

		tmp.clear();
		em.find_entities_by_tag("pickup", tmp);
		// SFG_TRACE("PICKUPs: {0}", tmp.size());
		for (int i = 0; i < tmp.size(); ++i)
		{
			// SFG_TRACE("PICKUP: {0}", i);
			auto		 name	= em.get_entity_meta(tmp[i]).name;
			world_handle visual = em.find_entity(tmp[i], "visual");
			// SFG_TRACE("VISUAL: {0}", !visual.is_null());

			pickup p = {.root_handle = tmp[i], .visual = visual};

			_pickups.push_back(p);
		}

		_cutscene_camera_waypoints.clear();
		_cutscene_camera_waypoint_index		 = 0;
		_cutscene_camera_waypoint_index_last = -1;
		_cutscene_camera_waypoint_t			 = 0.0f;
		_cutscene_camera_wait_remaining		 = 0.0f;
		_cutscene_camera_skip_segment		 = false;
		_cutscene_ui.reset();

		tmp.clear();
		em.find_entities_by_tag("camera_waypoint", tmp);
		std::sort(tmp.begin(), tmp.end(), [&](world_handle a, world_handle b) {
			const char* name_a = em.get_entity_meta(a).name;
			const char* name_b = em.get_entity_meta(b).name;
			if (name_a == nullptr && name_b == nullptr)
				return false;
			if (name_a == nullptr)
				return false;
			if (name_b == nullptr)
				return true;
			return std::strcmp(name_a, name_b) < 0;
		});
		SFG_TRACE("CAMERA WAYPOINTS: {0}", tmp.size());
		for (int i = 0; i < tmp.size(); ++i)
		{
			_cutscene_camera_waypoints.push_back(tmp[i]);
		}

		_cutscene_camera = em.find_entity_by_tag("cutscene_camera");
		if (!_cutscene_camera.is_null())
		{
			if (!_cutscene_camera_waypoints.empty())
			{
				const vector3 start_pos = em.get_entity_position_abs(_cutscene_camera_waypoints[0]);
				const quat	  start_rot = em.get_entity_rotation_abs(_cutscene_camera_waypoints[0]);
				em.set_entity_position_abs(_cutscene_camera, start_pos);
				em.set_entity_rotation_abs(_cutscene_camera, start_rot);

				world_handle camera_comp_handle = em.get_entity_component<comp_camera>(_cutscene_camera);
				if (!camera_comp_handle.is_null())
				{
					comp_camera& camera_comp = cm.get_component<comp_camera>(camera_comp_handle);
					camera_comp.set_main(w);
				}
			}

			if (_has_intro_cutscene)
				_cutscene_ui.init(w, _cutscene_camera);
		}
	}

	void gameplay::check_managed_entities_collision(world_handle e1, world_handle e2)
	{
		for (int i = 0; i < _managed_entities.size(); ++i)
		{
			managed_entity& ent = _managed_entities[i];

			if (!ent.params.destroy_on_collision)
				continue;

			if (ent.handle == e1 || ent.handle == e2)
			{
				SFG_TRACE("Managed entity collides");
				ent.marked_for_removal = true;
			}
		}
	}

	void gameplay::begin_managed_entities()
	{
		world&			  w	 = _app.get_world();
		resource_manager& rm = w.get_resource_manager();

		_managed_entities.clear();

		string_id bullet_template = "assets/entities/bullet.stkent"_hs;

		spawn_managed_entity(bullet_template, {2.0f, -5.0f, 0.0f}, quat::identity, bullet_params);

		// for (int i = 0; i < 100; ++i)
		// {
		// 	spawn_managed_entity(bullet_template, {2.0f * i, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f * i}, 20.0f - 0.1f * i);
		// }

		SFG_TRACE("begin_managed_entities");
	}

	world_handle gameplay::spawn_managed_entity(string_id resource, vector3 position, quat direction, const managed_entity_params& params)
	{
		world&			   w  = _app.get_world();
		resource_manager&  rm = w.get_resource_manager();
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();

		auto res = rm.get_resource_handle_by_hash_if_exists<entity_template>(resource);

		if (res.is_null())
		{
			SFG_ERR("can't find resource to spawn! {0}", resource);
			return {};
		}

		world_handle handle = em.instantiate_template(res);

		world_handle phys_comp_handle = em.get_entity_component<comp_physics>(handle);
		if (!phys_comp_handle.is_null())
		{
			comp_physics& phys_comp = cm.get_component<comp_physics>(phys_comp_handle);
			phys_comp.set_body_position_and_rotation(w, position, direction);
		}
		else
		{
			em.set_entity_position_abs(handle, position);
			em.set_entity_rotation_abs(handle, direction);
		}
		em.teleport_entity(handle);

		managed_entity ent = {};
		ent.handle		   = handle;
		ent.params		   = params;
		ent.t			   = 0.0f;

		_managed_entities.push_back(ent);

		return handle;
	}

	void gameplay::tick_managed_entities(float dt)
	{
		world&			   w  = _app.get_world();
		component_manager& cm = w.get_comp_manager();
		entity_manager&	   em = w.get_entity_manager();

		int i = 0;
		while (i < _managed_entities.size())
		{
			managed_entity& ent = _managed_entities[i];
			ent.t += dt;

			if (!em.is_valid(ent.handle) || ent.t >= ent.params.max_lifetime)
			{
				ent.marked_for_removal = true;
				i++;
				continue;
			}

			world_handle phys_comp_handle = em.get_entity_component<comp_physics>(ent.handle);
			if (!phys_comp_handle.is_null())
			{
				quat		  rot		= em.get_entity_rotation_abs(ent.handle);
				comp_physics& phys_comp = cm.get_component<comp_physics>(phys_comp_handle);
				vector3		  velocity	= rot.get_forward() * ent.params.speed;
				phys_comp.set_body_velocity(w, velocity);

				for (world_handle h : _all_enemies)
				{
					const vector3	v	 = em.get_entity_position(h);
					const vector3	me	 = em.get_entity_position(ent.handle);
					const float		dist = (me - v).magnitude_sqr();
					constexpr float trgt = 5.0f;
					if (dist < trgt * trgt)
					{
						world_handle eh = em.get_entity_component<comp_enemy_ai_basic>(h);
						if (!eh.is_null())
						{
							comp_enemy_ai_basic& ai = cm.get_component<comp_enemy_ai_basic>(eh);
							if (ai.get_state() == comp_enemy_ai_basic::enemy_state::dead)
								continue;

							ai.take_damage(w, 10);
							if (ai.get_state() == comp_enemy_ai_basic::enemy_state::dead)
							{
								// SPAWN PARTICLE
							}
						}
						// INAN: spawn & damage
						ent.marked_for_removal = true;
						break;
					}
				}

				if (ent.marked_for_removal)
				{
					i++;
					continue;
				}
			}
			i++;
		}

		for (int i = _managed_entities.size() - 1; i >= 0; --i)
		{
			managed_entity& ent = _managed_entities[i];

			if (ent.handle.is_null() || !em.is_valid(ent.handle))
			{
				_managed_entities.pop_back();
			}
			else if (ent.marked_for_removal)
			{
				em.destroy_entity(ent.handle);
				_managed_entities.pop_back();
			}
		}
	}
}
