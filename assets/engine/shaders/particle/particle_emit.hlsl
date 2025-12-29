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
#include "layout_defines_compute.hlsl"
#include "random.hlsl"
#include "particles.hlsl"

// ----------------------------------------------
// pass 1 - runs 64 threads per system
// emits new particles and appends to alive list b
// ----------------------------------------------
[numthreads(64, 1, 1)] void CSMain(uint3 gtid : SV_GroupThreadID,
                                   uint3 gid : SV_GroupID)
{
    // we dispatch 1, N, 1
    uint system_id = gid.x;

    // early out if system is not emitting.
    StructuredBuffer<uint> emit_counts = sfg_get_ssbo<uint>(sfg_rp_constant2);
    uint emit_count = emit_counts[system_id];
    if(emit_count == 0)
        return;

    // global
    ConstantBuffer<particle_pass_data> pass_params = sfg_get_cbv<particle_pass_data>(sfg_rp_constant0);

    // per particle
    RWStructuredBuffer<particle_state> states  = sfg_get_rws_buffer<particle_state>(sfg_rp_constant4);
    RWStructuredBuffer<uint> dead_indices = sfg_get_rws_buffer<uint>(sfg_rp_constant11);

    // per system
    RWStructuredBuffer<particle_system_data> system_data = sfg_get_rws_buffer<particle_system_data>(sfg_rp_constant2);
    RWStructuredBuffer<particle_emit_args> emit_args     = sfg_get_rws_buffer<particle_emit_args>(sfg_rp_constant3);
    
    uint max_particles = pass_params.max_particles_per_system;
    uint frame_index = pass_params.frame_index;
    particle_emit_args emit = emit_args[system_id];
    uint offset = max_particles * system_id;

    RWStructuredBuffer<uint> alive_list_b = sfg_get_rws_buffer<uint>(sfg_rp_constant10);
    RWByteAddressBuffer counters = sfg_get_rwb_buffer(sfg_rp_constant7);

    // thread 0 emits i=0,64,128,...
    // thread 1 emits i=1,65,129,...
    for (uint i = gtid.x; i < emit_count; i += 64)
    {
        uint old;
        InterlockedAdd(system_data[system_id].dead_count, 0xFFFFFFFFu, old); // -1
        if (old == 0)
        {
            // undo 
            InterlockedAdd(system_data[system_id].dead_count, 1u);
            break;
        }
        uint dead_local_index = old - 1;
        uint emitted_index = dead_indices[offset + dead_local_index];

        // emit state
        uint seed = (system_id * 9781u) ^ (i * 6271u) ^ (frame_index * 15643u);
        float random_pos_x = emit.min_pos.x + random01(seed * 1664325u) * (emit.max_pos.x - emit.min_pos.x);
        float random_pos_y = emit.min_pos.y + random01(seed * 1664625u) * (emit.max_pos.y - emit.min_pos.y);
        float random_pos_z = emit.min_pos.z + random01(seed * 1664125u) * (emit.max_pos.z - emit.min_pos.z);

        float random_vel_x = emit.min_vel.x + random01(seed * 1664545u) * (emit.max_vel.x - emit.min_vel.x);
        float random_vel_y = emit.min_vel.y + random01(seed * 1664595u) * (emit.max_vel.y - emit.min_vel.y);
        float random_vel_z = emit.min_vel.z + random01(seed * 1664125u) * (emit.max_vel.z - emit.min_vel.z);

        float random_c_x = emit.min_color.x + random01(seed * 1661525u) * (emit.max_color.x - emit.min_color.x);
        float random_c_y = emit.min_color.y + random01(seed * 1662525u) * (emit.max_color.y - emit.min_color.y);
        float random_c_z = emit.min_color.z + random01(seed * 1666525u) * (emit.max_color.z - emit.min_color.z);
        float random_c_w = emit.min_color.w + random01(seed * 1664585u) * (emit.max_color.w - emit.min_color.w);

        float random_lifetime = emit.min_pos.w +
                                random01(seed * 1650212u) * (emit.max_pos.w - emit.min_pos.w);

        float random_rotation = emit.min_vel.w + random01(seed * 165242u) * (emit.max_vel.w - emit.min_vel.w);

        // write particle state and mark alive.
        particle_state state = (particle_state)0;
        state.position_and_age = float4(random_pos_x, random_pos_y, random_pos_z, 0.0f);
        state.velocity_and_lifetime = float4(random_vel_x, random_vel_y, random_vel_z, random_lifetime);
        state.color = float4(random_c_x, random_c_y, random_c_z, random_c_w);
        state.rotation = random_rotation;
        state.system_id = system_id;
        states[emitted_index] = state;

        // write into alive_list_b.
        uint idx;
        counters.InterlockedAdd(4, 1, idx);
        if (idx < max_particles) alive_list_b[offset + idx] = emitted_index;
    }
}