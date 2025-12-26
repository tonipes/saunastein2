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


// ----------------------------------------------
// Constants / Params
// ----------------------------------------------

// sfg_rp_constant0 - ubo
// sfg_rp_constant1 - R32 depth full res
// sfg_rp_constant2 - RGB10A2 World Normals full res
// sfg_rp_constant3 - noise tex
// sfg_rp_constant4 - R8 unorm half_res ao output.

struct ao_params
{
    float4x4 proj;        // camera projection matrix
    float4x4 inv_proj;    // inverse of proj
    float4x4 view_matrix; // 3x3 of camera view.

    // Full-resolution render target size (camera buffer)
    uint2 full_size;           // e.g., (W, H)
    uint2 half_size;           // e.g., (W/2, H/2)
    float2 inv_full;           // 1/full_size
    float2 inv_half;           // 1/half_size

    // Projection
    float z_near;               // near plane
    float z_far;                // far plane 

    // AO controls
    float radius_world;         // AO radius in *world/view* units (meters)
    float bias;                 // small bias to reduce self-occlusion (e.g., 0.02)
    float intensity;            // AO strength (e.g., 1.2)
    float power;                // contrast shaping (e.g., 1.1)
    uint  num_dirs;             // e.g., 8
    uint  num_steps;            // e.g., 6
    float random_rot_strength;  // e.g., 1.0
};

struct pass_data
{
    uint system_count;
};

struct indirect_args
{
    uint vertex_count;
    uint start_vertex;
    uint instance_count;
    uint start_instance;
}

// ----------------------------------------------
// Main CS
// ----------------------------------------------
[numthreads(256, 1, 1)]
void CSMain(uint3 tid : SV_DispatchThreadID)
{
    uint id = tid.x;

    StructuredBuffer<pass_data> pass_params = sfg_get_ssbo(sfg_rp_constant0);

    if(id >= pass_params.system_count)
        return;

    RWStructuredBuffer<indirect_args> indirect_data = sfg_get_rw_buffer(sfg_rp_constant1);

    indirect_args args = indirect_data[id];
    args.instance_count = 0;
    args.start_instance = 0;
    args.vertex_count = 4;
    args.start_vertex = 0;
    indirect_data[id] = args;

}
