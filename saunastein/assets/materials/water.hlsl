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
#include "packing_utils.hlsl"
#include "entity.hlsl"
#include "normal.hlsl"
#include "bone.hlsl"
#include "render_pass_defines.hlsl"

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

#ifdef USE_ZPREPASS

struct vs_output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

#else

struct vs_output
{
    float4 pos : SV_POSITION;
    float3 world_norm : TEXCOORD0;
    float3 world_tan : TEXCOORD1;
    float3 world_bit : TEXCOORD2;
    float2 uv : TEXCOORD4;
};

#endif


//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

#ifdef USE_ZPREPASS

vs_output VSMain(vs_input IN)
{
    vs_output OUT;

    render_pass_data_opaque rp_data = sfg_get_cbv<render_pass_data_opaque>(sfg_rp_constant0);
    StructuredBuffer<gpu_entity> entity_buffer = sfg_get_ssbo<gpu_entity>(sfg_rp_constant1);

    gpu_entity entity = entity_buffer[sfg_object_constant0];

    float4 obj_pos;

    obj_pos = float4(IN.pos, 1.0f);

    float3 world_pos = mul(entity.model, obj_pos).xyz;
    OUT.pos = mul(rp_data.view_proj, float4(world_pos, 1.0f));
    OUT.uv = IN.uv;
    return OUT;
}

#else

vs_output VSMain(vs_input IN)
{
    vs_output OUT;

    render_pass_data_opaque rp_data = sfg_get_cbv<render_pass_data_opaque>(sfg_rp_constant0);
    StructuredBuffer<gpu_entity> entity_buffer = sfg_get_ssbo<gpu_entity>(sfg_rp_constant1);
    gpu_entity entity = entity_buffer[sfg_object_constant0];

    float4 obj_pos;
    float3 obj_norm;
    float3 obj_tan;

    obj_pos = float4(IN.pos, 1.0f);
    obj_norm = IN.normal;
    obj_tan = IN.tangent.xyz;

    float3 world_pos = mul(entity.model, obj_pos).xyz;
    float3 N = normalize(mul(entity.normal_matrix, float4(obj_norm, 1.0)).xyz);
    float3 T = normalize(mul(entity.normal_matrix, float4(obj_tan, 1.0)).xyz);

    // gram-schmidt ensure orthogonality.
    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T)) * IN.tangent.w;

    OUT.pos = mul(rp_data.view_proj, float4(world_pos, 1.0f));
    OUT.world_norm = N;
    OUT.world_tan = T;
    OUT.world_bit = B;
    OUT.uv = float2(world_pos.x, world_pos.z) * 0.1;

    return OUT;
}

#endif


//------------------------------------------------------------------------------
// Pixel Shader (G-Buffer)
//------------------------------------------------------------------------------


struct material_data
{
    float4 base_color_factor;
    float4 emissive_and_metallic_factor;
    float4 roughness_normal_strength_alpha;
};

struct texture_data
{
    uint gpu_index_albedo;
    uint gpu_index_normal;
    uint gpu_index_orm;
    uint gpu_index_emissive;
};

float2 hash2(float2 p)
{
    float2 k = float2(127.1f, 311.7f);
    float2 k2 = float2(269.5f, 183.3f);
    float2 s = float2(dot(p, k), dot(p, k2));
    return frac(sin(s) * 43758.5453f);
}

float2 rot2(float2 p, float a)
{
    float s = sin(a);
    float c = cos(a);
    return float2(c * p.x - s * p.y, s * p.x + c * p.y);
}

#ifdef WRITE_ID

uint PSMain(vs_output IN) : SV_TARGET
{
    return sfg_object_constant2;
}

#else

#if defined(USE_ZPREPASS) 

#ifdef USE_ALPHA_CUTOFF
void PSMain(vs_output IN)
{
    material_data mat_data = sfg_get_cbv<material_data>(sfg_mat_constant0);
    texture_data txt_data = sfg_get_cbv<texture_data>(sfg_mat_constant1);
    Texture2D tex_albedo = sfg_get_texture<Texture2D>(txt_data.gpu_index_albedo);
    SamplerState sampler_default = sfg_get_sampler_state(sfg_mat_constant2);

    float t = sfg_global_elapsed;
    float2 uv = IN.uv;
    uv += float2(t * 0.05f, t * 0.03f);
    uv += sin(float2(uv.y, uv.x) * 10.0f + t * 1.4f) * 0.025f;
    uv += sin(float2(uv.y, uv.x) * 22.0f - t * 2.3f) * 0.012f;

    float2 tile = floor(uv);
    float2 local_uv = frac(uv);
    float2 rnd = hash2(tile);
    float angle = (rnd.x * 2.0f - 1.0f) * 0.35f;
    float2 uv0 = rot2(local_uv - 0.5f, angle) + 0.5f + (rnd - 0.5f) * 0.12f;

    float2 tile_b = tile + float2(1.0f, 1.0f);
    float2 rnd_b = hash2(tile_b);
    float angle_b = (rnd_b.x * 2.0f - 1.0f) * 0.35f;
    float2 uv1 = rot2(local_uv - 0.5f, angle_b) + 0.5f + (rnd_b - 0.5f) * 0.12f;

    float blend = smoothstep(0.35f, 0.65f, rnd.y);
    float4 albedo_tex = lerp(
        tex_albedo.Sample(sampler_default, uv0),
        tex_albedo.Sample(sampler_default, uv1),
        blend
    );
	if(albedo_tex.a < mat_data.roughness_normal_strength_alpha.z)
	{
		discard;
	}
}
#endif

#else

struct ps_output
{
    float4 rt0 : SV_Target0; // Albedo
    float4 rt1 : SV_Target1; // World Normal
    float4 rt2 : SV_Target2; // orm
    float4 rt3 : SV_Target3; // Emissive
};

ps_output PSMain(vs_output IN)
{
    ps_output OUT;

    material_data mat_data = sfg_get_cbv<material_data>(sfg_mat_constant0);
    texture_data txt_data = sfg_get_cbv<texture_data>(sfg_mat_constant1);

    Texture2D tex_albedo = sfg_get_texture<Texture2D>(txt_data.gpu_index_albedo);
    SamplerState sampler_default = sfg_get_sampler_state(sfg_mat_constant2);

    float t = sfg_global_elapsed;
    float2 uv = IN.uv;
    uv += float2(t * 0.05f, t * 0.03f);
    uv += sin(float2(uv.y, uv.x) * 10.0f + t * 1.4f) * 0.025f;
    uv += sin(float2(uv.y, uv.x) * 22.0f - t * 2.3f) * 0.012f;

    float2 tile = floor(uv);
    float2 local_uv = frac(uv);
    float2 rnd = hash2(tile);
    float angle = (rnd.x * 2.0f - 1.0f) * 0.35f;
    float2 uv0 = rot2(local_uv - 0.5f, angle) + 0.5f + (rnd - 0.5f) * 0.12f;

    float2 tile_b = tile + float2(1.0f, 1.0f);
    float2 rnd_b = hash2(tile_b);
    float angle_b = (rnd_b.x * 2.0f - 1.0f) * 0.35f;
    float2 uv1 = rot2(local_uv - 0.5f, angle_b) + 0.5f + (rnd_b - 0.5f) * 0.12f;

    float blend = smoothstep(0.35f, 0.65f, rnd.y);
    float4 albedo_tex = lerp(
        tex_albedo.Sample(sampler_default, uv0),
        tex_albedo.Sample(sampler_default, uv1),
        blend
    );
    float4 albedo = albedo_tex * mat_data.base_color_factor;

    // OUT.rt0 = float4(frac(sfg_global_elapsed),2,0, 1.0);
    OUT.rt0 = albedo_tex;
    OUT.rt1 = float4(IN.world_norm, 0.0);
    OUT.rt2 = float4(1.0, 0.0, 0.0, 1.0);
    OUT.rt3 = float4(0,0,0, 1.0);
    


    return OUT;
}

#endif

#endif
