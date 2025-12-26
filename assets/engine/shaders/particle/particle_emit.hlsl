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
// Main CS
// ----------------------------------------------
[numthreads(64, 1, 1)]
void CSMain(uint3 tid : SV_DispatchThreadID)
{
    uint id = tid.x;

    particle_pass_data pass_params = sfg_get_cbv<particle_pass_data>(sfg_rp_constant0);
    if(id >= pass_params.system_count)
        return;

    StructuredBuffer<particle_emit_args> emit_data = sfg_get_ssbo<particle_emit_args>(sfg_rp_constant1);

    // initialized 0...4096, 0...4096 etc. indices. total count is max_systems * max_particles_per_system
    RWStructuredBuffer<uint> dead_list = sfg_get_rw_buffer<uint>(sfg_rp_constant2);

    // initialized all zeros. total count is max_systems * max_particles_per_system
    RWStructuredBuffer<uint> alive_list = sfg_get_rw_buffer<uint>(sfg_rp_constant3);

    // 1 uint per system
    RWStructuredBuffer<uint> dead_counters = sfg_get_rw_buffer<uint>(sfg_rp_constant4);
    RWStructuredBuffer<uint> alive_counters = sfg_get_rw_buffer<uint>(sfg_rp_constant5);

    // particle instance data.
    RWStructuredBuffer<particle_state> particle_states = sfg_get_rw_buffer<particle_state>(sfg_rp_constant6);

    uint dead_count = dead_counters[id];
    uint alive_count = alive_counters[id];
    particle_emit_args emit = emit_data[id];

    uint emit_count = emit.emit_count;

    uint start_index = id * pass_params.max_particles_per_system;

    [loop]
    for(uint i = 0; i < emit_count; i++)
    {
        uint off = (i + 1) * pass_params.frame_index;
        float random_pos_x = emit.min_x + random01(off * 1664325) * (emit.max_x - emit.min_x);
        float random_pos_y = emit.min_y + random01(off * 1664625) * (emit.max_y - emit.min_y);
        float random_pos_z = emit.min_z + random01(off * 1664125) * (emit.max_z - emit.min_z);

        float random_vel_x = emit.min_vel_x + random01(off * 1664545) * (emit.max_vel_x - emit.min_vel_x);
        float random_vel_y = emit.min_vel_y + random01(off * 1664595) * (emit.max_vel_y - emit.min_vel_y);
        float random_vel_z = emit.min_vel_z + random01(off * 1664125) * (emit.max_vel_z - emit.min_vel_z);
    
        float random_c_x = emit.min_color.x + random01(off* 1661525) * (emit.max_color.x - emit.min_color.x);
        float random_c_y = emit.min_color.y + random01(off* 1662525) * (emit.max_color.y - emit.min_color.y);
        float random_c_z = emit.min_color.z + random01(off* 1666525) * (emit.max_color.z - emit.min_color.z);
        float random_c_w = emit.min_color.w + random01(off* 1664585) * (emit.max_color.w - emit.min_color.w);
    
        float random_lifetime = emit.min_max_lifetime.x + random01(off * 1650212) * (emit.min_max_lifetime.y - emit.min_max_lifetime.x);
        
        // start stealing from the end of the dead buffer
        uint dead_index = start_index + dead_count;
        uint emitted_index = dead_list[dead_index];
        dead_list[dead_index] = 0;
        dead_count -= 1;

        uint alive_index = start_index + alive_count;
        alive_list[alive_index] = emitted_index;
        alive_count += 1;

        particle_state state = particle_states[emitted_index];
        state.position_and_age = float4(random_pos_x, random_pos_y, random_pos_z, 0.0f);
        state.velocity_and_lifetime = float4(random_vel_x, random_vel_y, random_vel_z, random_lifetime);
        state.color = float4(random_c_x, random_c_y, random_c_z, random_c_w);
        particle_states[emitted_index] = state;
    }

    dead_counters[id] = dead_count;
    alive_counters[id] = alive_count;
}
