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
	render_pass_data rp_ubo = sfg_get_rp_cbv<render_pass_data>();

	float4 worldPos = float4(IN.pos, 0.0f, 1.0f);
	OUT.pos = mul(rp_ubo.projection, worldPos);
	OUT.uv = IN.uv;
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