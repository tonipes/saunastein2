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

cbuffer MaterialCBV : material_ubo0
{
	float2 _screen_size;
}

Texture2D _txt_render_target : material_texture0;
SamplerState _smp_base : static_sampler_linear;

//------------------------------------------------------------------------------
// Pixel Shader: just output the interpolated vertex color
//------------------------------------------------------------------------------
float4 PSMain(VSOutput IN) : SV_TARGET
{
	float2 uv = IN.uv;

    // --- Barrel distortion ---
	const float distortionAmount = 0.0045f;
	const float baseAberration = 0.003; 
	
	float2 centeredUV = uv * 2.0f - 1.0f;
	float r2 = dot(centeredUV, centeredUV);
	centeredUV += centeredUV * r2 * distortionAmount;

    // --- Chromatic aberration (edge only) ---
	float edgeFactor = saturate((r2 - 0.85));
    // 0.0 near center, 1.0 near corners � tweak the 0.2/0.8

	float2 aberration_offset = centeredUV * baseAberration * edgeFactor;
	const float2 uvR = (centeredUV + aberration_offset) * 0.5f + 0.5f;
	const float2 uvG = centeredUV * 0.5f + 0.5f;
	const float2 uvB = (centeredUV - aberration_offset) * 0.5f + 0.5f;
	float4 sample_red = _txt_render_target.SampleLevel(_smp_base, uvR, 0);
	float red = sample_red.r;
	float green = _txt_render_target.SampleLevel(_smp_base, uvG, 0).g;
	float blue = _txt_render_target.SampleLevel(_smp_base, uvB, 0).b;
	float4 color = float4(red, green, blue, sample_red.a);
	
	float falloff = dot(centeredUV, centeredUV); // r^2 falloff
	float shading = saturate(1.0 - falloff * 0.15); // tweak 0.75 for how fast it darkens
	color.rgb *= shading;
	
    // --- Scanlines ---
	float pixel_y = IN.uv.y * _screen_size.y;
	if ((int) pixel_y % 2 == 0)
	{
		color.rgb *= 0.5;
	}

	return color;
}