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
#include "particles.hlsl"

// ----------------------------------------------
// pass 4 - 1 thread per alive particle
// check visibility, bump instance count and write to instance buffer for
// target system
// ----------------------------------------------
[numthreads(256,1,1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    // we dispatch ceil(alive_count / 256)
    uint alive_index = dtid.x;

    RWByteAddressBuffer counters = sfg_get_rwb_buffer(sfg_rp_constant7);
    uint aliveCount = counters.Load(4); 

    if (alive_index >= aliveCount) return;

    StructuredBuffer<uint> alive_list_b = sfg_get_ssbo<uint>(sfg_rp_constant8);
    uint particle_index = alive_list_b[alive_index];

    StructuredBuffer<particle_state> states  = sfg_get_ssbo<particle_state>(sfg_rp_constant4);
    ConstantBuffer<particle_pass_data> pass_params = sfg_get_cbv<particle_pass_data>(sfg_rp_constant0);
    
    // simple cull, no render if behind camera.
    float3 cam_to_particle = states[particle_index].position_and_age.xyz - pass_params.cam_pos_and_delta.xyz;
    float dp = dot(pass_params.cam_dir.xyz, cam_to_particle);
    uint visible = dp < 0.0 ? 0 : 1;

    if(visible == 0)
        return;

    // sort_keys[particle_index] = ~asuint(dot(cam_to_particle, cam_to_particle));

    uint system_id = states[particle_index].system_id;
    uint start = system_id * pass_params.max_particles_per_system;

    RWStructuredBuffer<particle_indirect_args> indirect_args     = sfg_get_rws_buffer<particle_indirect_args>(sfg_rp_constant5);
    
    uint idx;
    InterlockedAdd(indirect_args[system_id].instance_count, 1, idx);

    RWStructuredBuffer<particle_instance_data> instance_data    = sfg_get_rws_buffer<particle_instance_data>(sfg_rp_constant10);
    particle_instance_data pid = (particle_instance_data)0;
    pid.pos_and_rot = float4(states[particle_index].position_and_age.xyz, states[particle_index].rotation);
    pid.size = float4(1,0,0,0);
    instance_data[start + idx] = pid;
}