#include "layout_defines.hlsl"
#include "entity.hlsl"
#include "light.hlsl"
#include "pbr.hlsl"

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
    float4x4 invViewProj;
    float4 ambient_color_plights_count;
    float4 view_pos_slights_count;
    float dir_lights_count;
    uint pad[3];
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
	
    float2 pos = float2(
        (vertexID == 2) ? 3.0 : -1.0,
        (vertexID == 1) ? -3.0 : 1.0
    );
	
    OUT.pos = float4(pos, 0.0f, 1.0f);
    OUT.uv = 0.5f * (pos + 1.0f);
    OUT.uv.y = 1.0f - OUT.uv.y;
    return OUT;
}

//------------------------------------------------------------------------------
// Pixel Shader 
//------------------------------------------------------------------------------
float4 PSMain(vs_output IN) : SV_TARGET
{
    int2 ip = int2(IN.pos.xy);  

    float4 normal_data = tex_gbuffer_normal.Load(int3(ip, 0));
    float  d     = tex_gbuffer_depth.Load(int3(ip, 0)).r;
    float3 n = normal_data.xyz * 2.0f - 1.0;
    n = (dot(n,n) > 0.0f) ? normalize(n) : float3(0,0,1);

    // gbuffer fetch
    float4 albedo_data    = tex_gbuffer_color.SampleLevel(smp_linear, IN.uv, 0);
    float4 orm_data       = tex_gbuffer_orm.SampleLevel(smp_linear, IN.uv, 0);
    float4 emissive_data  = tex_gbuffer_emissive.SampleLevel(smp_linear, IN.uv, 0);

    // unpack
    float3 world_pos   = float3(0.0, 0.0, 0.0);
    float3 albedo      = albedo_data.xyz;

    return float4(d, d,d, 1.0f);


    // renormalize, kill quantization error.
    float  ao          = orm_data.x;
    float3 emissive    = emissive_data.xyz;

    // ambient (keep it simple; modulate by AO)
    float3 ambient_color = ambient_color_plights_count.xyz;
    float3 final_color   = albedo * ambient_color * ao + emissive;

    // view vector (not needed for pure lambert; kept if you add spec later)
    float3 view_pos = view_pos_slights_count.xyz;
    float3 V        = normalize(view_pos - world_pos);

    // point lights
    uint point_lights_count = (uint)ambient_color_plights_count.w;

    [loop]
    for (uint i = 0; i < point_lights_count; ++i)
    {
        gpu_point_light light = point_light_buffer[i];
    }

    return float4(final_color, 1.0f);
}


