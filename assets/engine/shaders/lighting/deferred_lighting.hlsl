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
#include "entity.hlsl"
#include "light.hlsl"
#include "pbr.hlsl"
#include "depth.hlsl"
#include "normal.hlsl"

//------------------------------------------------------------------------------
// In & Outs
//------------------------------------------------------------------------------

struct vs_output
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------

struct render_pass_data
{
    float4x4 inv_view_proj;
    float4 ambient_color_plights_count;
    float4 view_pos_slights_count;
    uint dir_lights_count;
    uint cascade_levels_gpu_index;
    uint cascades_count;
    float near_plane;
    float far_plane;
    float pad[3];
};

SamplerState smp_linear : static_sampler_linear;
SamplerState smp_nearest: static_sampler_nearest;
SamplerComparisonState smp_shadow : static_sampler_shadow_2d;

//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

vs_output VSMain(uint vertexID : SV_VertexID)
{
    vs_output OUT;
	
    float2 pos;
    if (vertexID == 0) pos = float2(-1.0, -1.0);
    else if (vertexID == 1) pos = float2(-1.0,  3.0);
    else                  pos = float2( 3.0, -1.0);

    OUT.pos = float4(pos, 0.0, 1.0);
    OUT.uv  = pos * float2(0.5, -0.5) + 0.5;
    return OUT;
}


//------------------------------------------------------------------------------
// Pixel Shader 
//------------------------------------------------------------------------------
float4 PSMain(vs_output IN) : SV_TARGET
{
    
    render_pass_data rp_data = sfg_get_cbv<render_pass_data>(sfg_rp_constant0);
    StructuredBuffer<gpu_entity> entity_buffer = sfg_get_ssbo<gpu_entity>(sfg_rp_constant1);
    StructuredBuffer<gpu_point_light> point_light_buffer = sfg_get_ssbo<gpu_point_light>(sfg_rp_constant2);
    StructuredBuffer<gpu_spot_light> spot_light_buffer = sfg_get_ssbo<gpu_spot_light>(sfg_rp_constant3);
    StructuredBuffer<gpu_dir_light> dir_light_buffer = sfg_get_ssbo<gpu_dir_light>(sfg_rp_constant4);
    StructuredBuffer<gpu_shadow_data> shadow_data_buffer = sfg_get_ssbo<gpu_shadow_data>(sfg_rp_constant5);
    StructuredBuffer<float> float_buffer = sfg_get_ssbo<float>(sfg_rp_constant6);
    Texture2D tex_gbuffer_color = sfg_get_texture<Texture2D>(sfg_rp_constant7);
    Texture2D tex_gbuffer_normal = sfg_get_texture<Texture2D>(sfg_rp_constant8);
    Texture2D tex_gbuffer_orm = sfg_get_texture<Texture2D>(sfg_rp_constant9);
    Texture2D tex_gbuffer_emissive = sfg_get_texture<Texture2D>(sfg_rp_constant10);
    Texture2D tex_gbuffer_depth = sfg_get_texture<Texture2D>(sfg_rp_constant11);
    Texture2D tex_ao = sfg_get_texture<Texture2D>(sfg_rp_constant12);

     // pixel for .Load()
    int2 ip = int2(IN.pos.xy);

    // fetch depth
    float device_depth = tex_gbuffer_depth.Load(int3(ip, 0)).r;

    // background early-out (reversed-Z)
    if (is_background(device_depth))
       {
          float2 uv = IN.uv;

        // --- World-space view ray ---
        float3 camPos = rp_data.view_pos_slights_count.xyz;
        // Use depth 0.0 (will still lie on the correct ray whether reversed-Z or not)
        float3 worldPosOnRay = reconstruct_world_position(uv, 0.0, rp_data.inv_view_proj);
        float3 viewDir = normalize(worldPosOnRay - camPos);  // world-space view direction

        // --- Simple vertical gradient in screen-space (sunrise-ish) ---
        float3 bottomColor = float3(0.2, 0.1, 0.2); // orange near horizon
        float3 midColor    = float3(0.1, 0.1, 0.2); // peach
        float3 topColor    = float3(0.2, 0.10, 0.1); // deep blue

        // uv.y: 0 = top, 1 = bottom
        float t = saturate(1.0 - uv.y); // 0 bottom, 1 top

        float midBlend = saturate((t - 0.3) / 0.4);
        float3 lowSky  = lerp(bottomColor, midColor, midBlend);
        float3 skyCol  = lerp(lowSky,     topColor,  saturate((t - 0.7) / 0.3));

        // --- World-space circular sun ---

        // Hardcoded world-space sun direction (rising a bit above horizon)
        // Think of this as “where in the world the sun actually is”.
        float3 sunDir = normalize(float3(0.3, 0.4, -0.8));  // tweak to taste

        // Cosine of angle between view ray and sun direction
        float cosTheta = dot(normalize(viewDir), sunDir);

        // Hardcoded angular radius & softness via cosine thresholds
        // (values very close to 1.0 => small angles)
        float innerCos = 0.9995; // core radius
        float outerCos = 0.9980; // soft edge

        // 0 outside, 1 at center; perfectly circular on the unit sphere
        float sunFactor = smoothstep(outerCos, innerCos, cosTheta);

        // Make center a bit “punchier”
        float innerCoreCos = 0.9998;
        float coreFactor = smoothstep(innerCos, innerCoreCos, cosTheta);
        sunFactor = max(sunFactor, coreFactor);

        float3 sunColor = float3(1.0, 0.95, 0.75)* 200;

        // Additive glow over the gradient

        return float4(skyCol, 1.0);
       }

    
    // reconstruct world position
    float2 uv = IN.uv;
    float3 world_pos = reconstruct_world_position(uv, device_depth, rp_data.inv_view_proj);
    float3 V = normalize(rp_data.view_pos_slights_count.xyz - world_pos);

    // decode gbuffer
    float4 albedo_data   = tex_gbuffer_color.SampleLevel(smp_nearest, uv, 0);      // sRGB RT was linearized when written; this is now linear
    float4 normal_data = tex_gbuffer_normal.Load(int3(ip, 0));          // RGB10A2_UNORM -> oct encode
    float4 orm_data      = tex_gbuffer_orm.Load(int3(ip, 0));      // [ao, rough, metal, _]
    float4 emissive_data = tex_gbuffer_emissive.Load(int3(ip, 0));    // linear
    float3 albedo = albedo_data.xyz;
    float3 emissive = emissive_data.xyz;
    float  ao        = saturate(orm_data.r) * tex_ao.Load(int3(ip, 0)).r;
    float  roughness = saturate(orm_data.g);
    float  metallic  = saturate(orm_data.b);
    float3 N = oct_decode(normal_data.xy);

    float3 ambientColor = rp_data.ambient_color_plights_count.xyz;
    float3 lighting = ambientColor * albedo * ao;

    // fetch light prep
    uint point_light_count = uint(rp_data.ambient_color_plights_count.w);
    uint spot_light_count = uint(rp_data.view_pos_slights_count.w);
    uint dir_light_count = rp_data.dir_lights_count;

    [loop]
    for(uint i = 0; i < point_light_count; i++)
    {
        gpu_point_light light = point_light_buffer[i];
        gpu_entity entity = entity_buffer[uint(light.color_entity_index.w)];
        float3 light_col = light.color_entity_index.xyz;
        float3 light_pos = entity.position.xyz;
        float intensity = light.intensity_range.x;
        float range = light.intensity_range.y ;

        float3 Lvec = light_pos - world_pos;
        float  d  = length(Lvec);
        float  dist = max(d, 1e-4);
        float3 L    = Lvec / dist;
        
        float  att = attenuation(range, d);
        float3 radiance  = light_col * (intensity * att);

        float shadow_vis = 1.0;
        int shadow_data_index = (int)light.shadow_resolution_map_and_data_index.w;

        if(shadow_data_index != -1)
        {
            int shadow_map_index = (int)light.shadow_resolution_map_and_data_index.z;

            TextureCube shadow_map = sfg_get_texture<TextureCube>(shadow_map_index);
            gpu_shadow_data sd = shadow_data_buffer[shadow_data_index];
            float4x4 light_space = sd.light_space_matrix;
            float2 shadow_resolution = light.shadow_resolution_map_and_data_index.xy;

            shadow_vis = sample_shadow_cube(shadow_map, smp_shadow, light_space, light_pos, world_pos, N, L, shadow_resolution, light.near_plane,light.far_plane);
        }

        lighting += calculate_pbr(V, N, L, albedo, ao, roughness, metallic, radiance * shadow_vis);
    }

    [loop]
    for(uint i = 0; i < spot_light_count; i++)
    {
        gpu_spot_light light = spot_light_buffer[i];                    // FIXED
        gpu_entity e = entity_buffer[uint(light.color_entity_index.w)];
        float3 entity_forward = e.forward.xyz;
        float3 light_col = light.color_entity_index.xyz;
        float3 light_pos = e.position.xyz;
        float  intensity = light.intensity_range_inner_outer.x;
        float  range     = light.intensity_range_inner_outer.y;
        float  cosInner  = saturate(light.intensity_range_inner_outer.z);
        float  cos_outer  = saturate(light.intensity_range_inner_outer.w);

        // To the shaded point
        float3 Lvec = light_pos - world_pos;
        float  d    = length(Lvec);
        float  dist = max(d, 1e-4);
        float3 L    = Lvec / dist;

        // Distance attenuation (your helper)
        float  attDist = attenuation(range, d);

        // Angular attenuation
        float  cosTheta = dot(normalize(-L), entity_forward); 
        float cosInnerEff = compute_cosInner(cosInner, cos_outer, g_default_spot_blend);
        float cone = spot_blend_hermite(cosTheta, cos_outer, cosInnerEff, g_softness_exp);

        // Final spotlight attenuation
        float  att = attDist * cone;
        float shadow_vis = 1.0;

        int shadow_data_index = (int)light.shadow_resolution_map_and_data_index.w;

        if(shadow_data_index != -1)
        {
            int shadow_map_index = (int)light.shadow_resolution_map_and_data_index.z;

            Texture2D shadow_map = sfg_get_texture<Texture2D>(shadow_map_index);
            gpu_shadow_data sd = shadow_data_buffer[shadow_data_index];
            float4x4 light_space = sd.light_space_matrix;
            float2 shadow_resolution = light.shadow_resolution_map_and_data_index.xy;

            shadow_vis = sample_shadow_cone(shadow_map, smp_shadow, light_space, world_pos, N, L, entity_forward, shadow_resolution, cos_outer);
        }

        if (att > 0.0f)
        {
            float3 radiance = light_col * (intensity * att);
            lighting += calculate_pbr(V, N, L, albedo, ao, roughness, metallic, radiance * shadow_vis);
        }
    }

    float linear_depth = linearize_depth(device_depth, rp_data.near_plane, rp_data.far_plane);
    int layer = -1;
    [loop]
    for(int i = 0; i < rp_data.cascades_count; i++)
    {
        if(linear_depth < float_buffer[rp_data.cascade_levels_gpu_index + i] )
        {
            layer = i;
            break;
        }
    }
    if(layer == -1)
    {
        layer = int(rp_data.cascades_count);
    }

    [loop]
    for (uint i = 0; i < dir_light_count; i++)
    {
        gpu_dir_light light = dir_light_buffer[i];
        gpu_entity e = entity_buffer[uint(light.color_entity_index.w)];
        float3 light_col = light.color_entity_index.xyz;

        float3 L = normalize(-e.forward.xyz);       

        float intensity = light.intensity.x; 

        float3 radiance = light_col * intensity; 

        int shadow_data_index = (int)light.shadow_resolution_map_and_data_index.w;

        float shadow_vis = 1.0f;

        if(shadow_data_index != -1)
        {
            int shadow_map_index = (int)light.shadow_resolution_map_and_data_index.z;

            Texture2DArray shadow_map = sfg_get_texture<Texture2DArray>(shadow_map_index);
            gpu_shadow_data sd_curr = shadow_data_buffer[shadow_data_index + layer];
            float4x4 light_space_curr = sd_curr.light_space_matrix;
            float2 shadow_resolution = light.shadow_resolution_map_and_data_index.xy;
            shadow_vis = sample_cascade_shadow(
                    shadow_map, smp_shadow,
                    light_space_curr,
                    world_pos, N, L,
                    layer,
                    shadow_resolution,
                    sd_curr.texel_world
                );
        }
       lighting += calculate_pbr(V, N, L, albedo, ao, roughness, metallic, radiance * shadow_vis);
    }

    lighting += emissive;

    //return float4((float)layer / (float)rp_data.cascades_count,0,0,1);
    return float4(lighting, 1.0f);
}


