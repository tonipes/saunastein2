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
#include "layout_defines.hlsl"
#include "particle/particles.hlsl"
#include "packing_utils.hlsl"

//------------------------------------------------------------------------------
// In & Outs
//------------------------------------------------------------------------------

#ifdef USE_SKINNING

struct vs_input
{
	float3 pos : POSITION;
	float3 normal : NORMAL0;
	float4 tangent : TANGENT0;
	float2 uv : TEXCOORD0;
    float4 bone_weights : BLENDWEIGHT0;
    uint4  bone_indices : BLENDINDICES0;
};

#else

struct vs_input
{
    float3 pos : POSITION;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
};

#endif

struct vs_output
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD4;
};

//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

static const float2 k_corners[4] = {
    float2(-1, -1),
    float2(-1, +1),
    float2(+1, -1),
    float2(+1, +1)
};

static const float2 k_uvs[4] = {
    float2(0, 1),
    float2(0, 0),
    float2(1, 1),
    float2(1, 0)
};

vs_output VSMain(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    vs_output o;

    StructuredBuffer<particle_instance_data> instance_data    = sfg_get_ssbo<particle_instance_data>(sfg_rp_constant1);
    ConstantBuffer<particle_pass_data> pass_params = sfg_get_cbv<particle_pass_data>(sfg_rp_constant0);

    particle_instance_data p = instance_data[iid];

    float3 center = p.pos_rot_size.xyz;
    float2 rot_size = unpack_rot_size((uint)p.pos_rot_size.w);
    float4 color = unpack_rgba8_unorm(p.color);
   
    // Build billboard basis. Use camera dir (or compute to-camera if desired).
    float3 forward = normalize(pass_params.cam_dir.xyz);

    // Choose a stable world up; if forward ~ up, pick another.
    float3 world_up = float3(0, 1, 0);
    if (abs(dot(forward, world_up)) > 0.99)
        world_up = float3(0, 0, 1);

    float3 right = normalize(cross(world_up, forward));
    float3 up    = cross(forward, right);

    // Optional spin around forward using p.pos_and_rot.w (radians)
    float s = sin(rot_size.x);
    float c = cos(rot_size.x);
    float3 right2 = right * c + up * s;
    float3 up2    = up    * c - right * s;

    float2 corner = k_corners[vid];
    float  half_size = 0.5 * rot_size.y;

    float3 world_pos =
        center +
        right2 * (corner.x * half_size) +
        up2    * (corner.y * half_size);

    o.pos =  mul(pass_params.view_proj, float4(world_pos, 1.0f));
    o.uv   = k_uvs[vid];
    o.color = color;
    return o;
}

//------------------------------------------------------------------------------
// Pixel Shader 
//------------------------------------------------------------------------------

float4 PSMain(vs_output IN) : SV_TARGET
{
    return IN.color;
}


