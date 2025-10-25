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

cbuffer render_pass : render_pass_ubo0
{
    float4x4 inv_view_proj;
    float4 ambient_color_plights_count;
    float4 view_pos_slights_count;
    float dir_lights_count;
    float pad[7];
};

StructuredBuffer<gpu_entity> entity_buffer : render_pass_ssbo0;
StructuredBuffer<gpu_point_light> point_light_buffer : render_pass_ssbo1;
StructuredBuffer<gpu_spot_light> spot_light_buffer : render_pass_ssbo2;
StructuredBuffer<gpu_dir_light> dir_light_buffer : render_pass_ssbo3;

Texture2D tex_gbuffer_color : render_pass_texture0;
Texture2D tex_gbuffer_normal: render_pass_texture1;
Texture2D tex_gbuffer_orm : render_pass_texture2;
Texture2D tex_gbuffer_emissive : render_pass_texture3;
Texture2D tex_gbuffer_depth : render_pass_texture4;
SamplerState smp_linear : static_sampler_linear;
SamplerState smp_nearest: static_sampler_nearest;

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

// Very small epsilon guard
static bool is_background(float device_depth)
{
    // Reversed-Z: far = 0, near = 1. Treat ~0 as background
    return device_depth <= 1e-6;
}

//------------------------------------------------------------------------------
// Pixel Shader 
//------------------------------------------------------------------------------
float4 PSMain(vs_output IN) : SV_TARGET
{
     // pixel for .Load()
    int2 ip = int2(IN.pos.xy);

    // fetch depth
    float device_depth = tex_gbuffer_depth.Load(int3(ip, 0)).r;

    // background early-out (reversed-Z)
    if (is_background(device_depth))
        return float4(0,0,0,1);

    // reconstruct world position
    float2 uv = IN.uv;
    float3 world_pos = reconstruct_world_position(uv, device_depth, inv_view_proj);
    float3 V = normalize(view_pos_slights_count.xyz - world_pos);

    // decode gbuffer
    float4 albedo_data   = tex_gbuffer_color.SampleLevel(smp_nearest, uv, 0);      // sRGB RT was linearized when written; this is now linear
    float4 normal_data = tex_gbuffer_normal.Load(int3(ip, 0));          // RGB10A2_UNORM -> oct encode
    float4 orm_data      = tex_gbuffer_orm   .Load(int3(ip, 0));      // [ao, rough, metal, _]
    float4 emissive_data = tex_gbuffer_emissive.Load(int3(ip, 0));    // linear
    float3 albedo = albedo_data.xyz;
    float3 emissive = emissive_data.xyz;
    float  ao        = saturate(orm_data.r);
    float  roughness = saturate(orm_data.g);
    float  metallic  = saturate(orm_data.b);
    float3 N = oct_decode(normal_data.xy);

    float3 ambientColor = ambient_color_plights_count.xyz;
    float3 lighting = ambientColor * albedo * ao;

    // fetch light prep
    uint point_light_count = uint(ambient_color_plights_count.w);
    uint spot_light_count = uint(view_pos_slights_count.w);
    uint dir_light_count = uint(dir_lights_count);

    [loop]
    for(uint i = 0; i < point_light_count; i++)
    {
        gpu_point_light light = point_light_buffer[i];
        gpu_entity entity = entity_buffer[uint(light.color_entity_index.w)];
        float3 light_col = light.color_entity_index.xyz;
        float3 light_pos = entity.position.xyz;
        float intensity = light.intensity_range.x;
        float range = light.intensity_range.y;

        float3 Lvec = light_pos - world_pos;
        float  d  = length(Lvec);
        float  dist = max(d, 1e-4);
        float3 L    = Lvec / dist;
        
        float  att = attenuation(range, d);
        float3 radiance  = light_col * (intensity * att);
        lighting += calculate_pbr(V, N, L, albedo, ao, roughness, metallic, radiance);
    }

    [loop]
    for(uint i = 0; i < spot_light_count; i++)
    {
        gpu_spot_light light = spot_light_buffer[i];                    // FIXED
        gpu_entity e = entity_buffer[uint(light.color_entity_index.w)];
        float3 light_col = light.color_entity_index.xyz;
        float3 light_pos = e.position.xyz;
        float  intensity = light.intensity_range_inner_outer.x;
        float  range     = light.intensity_range_inner_outer.y;
        float  cosInner  = saturate(light.intensity_range_inner_outer.z);
        float  cosOuter  = saturate(light.intensity_range_inner_outer.w);

        // To the shaded point
        float3 Lvec = light_pos - world_pos;
        float  d    = length(Lvec);
        float  dist = max(d, 1e-4);
        float3 L    = Lvec / dist;

        // Distance attenuation (your helper)
        float  attDist = attenuation(range, d);

        // Angular attenuation
        float  cosTheta = dot(normalize(-L), e.forward.xyz); 
        float cosInnerEff = compute_cosInner(cosInner, cosOuter, g_default_spot_blend);
        float cone = spot_blend_hermite(cosTheta, cosOuter, cosInnerEff, g_softness_exp);

        // Final spotlight attenuation
        float  att = attDist * cone;

        if (att > 0.0f)
        {
            float3 radiance = light_col * (intensity * att);
            lighting += calculate_pbr(V, N, L, albedo, ao, roughness, metallic, radiance);
        }
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
        lighting += calculate_pbr(V, N, L, albedo, ao, roughness, metallic, radiance);
    }

    lighting += emissive;

    return float4(lighting, 1.0f);
}


