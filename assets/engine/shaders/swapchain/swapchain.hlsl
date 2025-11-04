#include "../layout_defines.hlsl"

//------------------------------------------------------------------------------
// In & Outs
//------------------------------------------------------------------------------

struct VSOutput
{
	float4 pos : SV_POSITION; // transformed position
	float2 uv : TEXCOORD0; // pass-through UV
};

//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

VSOutput VSMain(uint vertexID : SV_VertexID)
{
	VSOutput OUT;
	
	float2 pos = float2(
        (vertexID == 2) ? 3.0 : -1.0,
        (vertexID == 1) ? -3.0 : 1.0
    );
	
	OUT.pos = float4(pos, 0.0f, 1.0f);
	OUT.uv = 0.5f * (pos + 1.0f);
	OUT.uv.y = 1.0f - OUT.uv.y;
	return OUT;
}

SamplerState sampler_base : static_sampler_linear;

//------------------------------------------------------------------------------
// Pixel Shader
//------------------------------------------------------------------------------
float4 PSMain(VSOutput IN) : SV_TARGET
{
	Texture2D txt_debug_controller = sfg_get_texture<Texture2D>(sfg_rp_constant0);
	Texture2D txt_world = sfg_get_texture<Texture2D>(sfg_rp_constant1);

	float4 color_debug_controller = txt_debug_controller.SampleLevel(sampler_base, IN.uv, 0);
	float4 color_world = txt_world.SampleLevel(sampler_base, IN.uv, 0);
	return color_debug_controller + color_world;
}