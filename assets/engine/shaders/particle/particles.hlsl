// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//  This file is a part of: Stakeforge Engine
//  https://github.com/inanevin/StakeforgeEngine
//  
//  Author: Inan Evin
//  http://www.inanevin.com
//  
//  Copyright (c) [2025-] [Inan Evin]
//  
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//  
//     1. Redistributions of source code must retain the above copyright notice, this
//        list of conditions and the following disclaimer.
//  
//     2. Redistributions in binary form must reproduce the above copyright notice,
//        this list of conditions and the following disclaimer in the documentation
//        and/or other materials provided with the distribution.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
//  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
//  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
//  OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct particle_pass_data
{
    float4x4 view_proj;
    float4 cam_pos_and_delta;
    float4 cam_dir;
    uint max_particles_per_system;
    uint frame_index;
    uint max_systems;
    uint num_systems;
};

struct particle_system_data
{
    uint alive_count;
    uint dead_count;
};


struct particle_emit_args
{
    float4 min_color;
    float4 max_color;
    float4 min_max_size_and_size_velocity;
    float4 min_max_angular_and_opacity_velocity;
    float4 min_pos; // w is min lifetime
    float4 max_pos; // w is max lifetime
    float4 min_vel; // w is min rotation
    float4 max_vel; // w is max rotation
};

struct particle_state
{
    float4 position_and_age;
    float4 velocity_and_lifetime;
    float opacity_velocity;
    uint size_and_size_velocity;
    uint rotation_angular_velocity;
    uint color;
    uint system_id;
};

struct particle_instance_data
{
    float4 pos_rot_size;
    uint color;
};

struct particle_counters
{
    uint alive_count_a;
    uint alive_count_b;
};

struct particle_indirect_args
{
    uint vertex_count;
    uint instance_count;
    uint start_vertex;
    uint start_instance;
};

struct particle_sim_count_args
{
    uint group_sim_x;
    uint group_sim_y;
    uint group_sim_z;
    uint group_count_x;
    uint group_count_y;
    uint group_count_z;
};
