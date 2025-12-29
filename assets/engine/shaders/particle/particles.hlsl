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
    float4 min_pos; // w is min lifetime
    float4 max_pos; // w is max lifetime
    float4 min_vel; // w is min rotation
    float4 max_vel; // w is max rotation
};

struct particle_state
{
    float4 position_and_age;
    float4 color;
    float4 velocity_and_lifetime;
    float rotation;
    uint system_id;
    float padding[2];
};

struct particle_instance_data
{
    float4 pos_and_rot;
    float4 size;
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

struct particle_sim_args
{
    uint dispatch_x;
    uint dispatch_y;
    uint dispatch_z;
};

struct particle_count_args
{
    uint dispatch_x;
    uint dispatch_y;
    uint dispatch_z;
};

/*
ConstantBuffer<particle_pass_data> pass_params = sfg_get_cbv<particle_pass_data>(sfg_rp_constant0);
StructuredBuffer<uint> emit_counts = sfg_get_ssbo<uint>(sfg_rp_constant2);

RWStructuredBuffer<particle_system_data> system_data = sfg_get_rws_buffer<particle_system_data>(sfg_rp_constant2);
RWStructuredBuffer<particle_emit_args> emit_args     = sfg_get_rws_buffer<particle_emit_args>(sfg_rp_constant3);
RWStructuredBuffer<particle_state> states  = sfg_get_rws_buffer<particle_state>(sfg_rp_constant4);

RWStructuredBuffer<particle_indirect_args> indirect_args     = sfg_get_rws_buffer<particle_indirect_args>(sfg_rp_constant5);
RWByteAddressBuffer sim_indirect_args     = sfg_get_rwb_buffer(sfg_rp_constant6);
RWByteAddressBuffer counters = sfg_get_rwb_buffer(sfg_rp_constant7);

RWStructuredBuffer<uint> alive_list_a = sfg_get_rws_buffer<uint>(sfg_rp_constant9);
RWStructuredBuffer<uint> alive_list_b = sfg_get_rws_buffer<uint>(sfg_rp_constant10);
RWStructuredBuffer<uint> dead_indices = sfg_get_rws_buffer<uint>(sfg_rp_constant11);

RWStructuredBuffer<particle_instance_data> instance_data    = sfg_get_rws_buffer<particle_instance_data>(sfg_rp_constant12);
RWByteAddressBuffer count_indirect_args     = sfg_get_rwb_buffer(sfg_object_constant0);

*/