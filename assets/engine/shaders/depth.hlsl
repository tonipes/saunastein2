// projA = P[2][2], projB = P[3][2]
float linear_eye_depth(float deviceDepth, float projA, float projB)
{
    return projB / max(deviceDepth - projA, 1e-6);
}

float3 world_pos_from_depth(float2 uv, float depth, float4x4 invViewProj)
{
    // from UV+depth to clip
    float4 clip = float4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    float4 world = mul(invViewProj, clip);
    return world.xyz / world.w;
}