#include "layout_defines.hlsl"

//------------------------------------------------------------------------------
// In & Outs
//------------------------------------------------------------------------------

struct VSInput
{
	float3 pos : POSITION; // XY position
	float4 color : COLOR0; // RGBA vertex color
};


struct VSOutput
{
	float4 pos : SV_POSITION; // transformed position
	float4 color : COLOR0; // pass-through color
};

//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

struct render_pass_data
{
	float4x4 view;
	float4x4 proj;
	float4x4 view_proj;
	float2 resolution;
};

VSOutput VSMain(VSInput IN)
{
	VSOutput OUT;
	render_pass_data rp = sfg_get_cbv<render_pass_data>(sfg_rp_constant0);

    OUT.pos = mul(rp.view_proj, float4(IN.pos, 1.0f));
	OUT.color = IN.color;

	return OUT;
}

//------------------------------------------------------------------------------
// Pixel Shader: just output the interpolated vertex color
//------------------------------------------------------------------------------
float4 PSMain(VSOutput IN) : SV_TARGET
{
	return IN.color;
}