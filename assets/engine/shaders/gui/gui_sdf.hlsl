#include "../layout_defines.hlsl"

//------------------------------------------------------------------------------
// In & Outs
//------------------------------------------------------------------------------

struct VSInput
{
	float2 pos : POSITION; // XY position
	float2 uv : TEXCOORD0; // UV coords (unused in PS here, but available)
	float4 color : COLOR0; // RGBA vertex color
};


struct VSOutput
{
	float4 pos : SV_POSITION; // transformed position
	float2 uv : TEXCOORD0; // pass-through UV
	float4 color : COLOR0; // pass-through color
};

//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

struct render_pass_data
{
	float4x4 projection;
	float sdf_thickness;
	float sdf_softness;
};

VSOutput VSMain(VSInput IN)
{
	VSOutput OUT;
    
	render_pass_data rp_data = sfg_get_cbv<render_pass_data>(sfg_rp_constant0);

	float4 worldPos = float4(IN.pos, 0.0f, 1.0f);
	OUT.pos = mul(rp_data.projection, worldPos);
	OUT.uv = IN.uv;
	OUT.color = IN.color;
	return OUT;
}

//------------------------------------------------------------------------------
// Pixel Shader
//------------------------------------------------------------------------------

SamplerState sampler_base : static_sampler_gui_text;

float4 PSMain(VSOutput IN) : SV_TARGET
{
	render_pass_data rp_data = sfg_get_cbv<render_pass_data>(sfg_rp_constant0);
	Texture2D txt_atlas = sfg_get_texture2D(sfg_object_constant0);
	
	float distance = txt_atlas.SampleLevel(sampler_base, IN.uv, 0).x;
	float alpha = smoothstep(rp_data.sdf_thickness - rp_data.sdf_softness, rp_data.sdf_thickness + rp_data.sdf_softness, distance);
	return float4(IN.color.xyz, alpha);
}