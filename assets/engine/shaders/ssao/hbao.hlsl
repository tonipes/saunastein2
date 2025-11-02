#include "layout_defines_compute.hlsl"
#include "normal.hlsl"

// =====================================================================================
// HBAO Half-Resolution Compute (Option A: sample full-res depth/normals)
// File: AO_Half.hlsl
// =====================================================================================

#ifndef PI
#define PI 3.14159265358979323846
#endif

// ----------------------------------------------
// Constants / Params
// ----------------------------------------------

// sfg_rp_constant0 - R32 depth full res
// sfg_rp_constant1 - RGB10A2 World Normals full res
// sfg_rp_constant2 - noise tex
// sfg_rp_constant3 - R8 unorm half_res ao output.

struct ao_params
{
    // Full-resolution render target size (camera buffer)
    uint2 full_size;           // e.g., (W, H)
    uint2 half_size;           // e.g., (W/2, H/2)
    float2 inv_full;           // 1/full_size
    float2 inv_half;           // 1/half_size

    // Projection
    float fx;                   // proj[0][0]
    float fy;                   // proj[1][1]
    float z_near;               // near plane (if you linearize from device depth)
    float z_far;                // far plane  (if you linearize from device depth)

    // AO controls
    float radius_world;         // AO radius in *world/view* units (meters)
    float bias;                 // small bias to reduce self-occlusion (e.g., 0.02)
    float intensity;            // AO strength (e.g., 1.2)
    float power;                // contrast shaping (e.g., 1.1)
    uint  num_dirs;             // e.g., 8
    uint  num_steps;            // e.g., 6
    float random_rot_strength;  // e.g., 1.0

    float3x3 world_to_view_rot; // 3x3 of camera view.
};

// ----------------------------------------------
// Resources
// ----------------------------------------------

SamplerState smp_point  : static_sampler_nearest;
SamplerState smp_linear : static_sampler_linear;

// ----------------------------------------------
// Helpers
// ----------------------------------------------

// Convert device depth [0,1] to view-space Z (negative forward), given fx/fy and near/far.
float depth_to_view_z(float z01, float z_near, float z_far)
{
    // Standard reverse mapping for a typical projection.
    float z = z01 * 2.0f - 1.0f; // NDC z
    // Reconstruct view z from NDC z for a standard projection matrix:
    // z_ndc = (a * z_view + b) / (-z_view)
    // where a = (f + n)/(f - n), b = (2fn)/(f - n). Solve for z_view:
    float a = (z_far + z_near) / (z_far - z_near);
    float b = (2.0f * z_far * z_near) / (z_far - z_near);
    // Avoid div by zero
    float vz = b / (z + a);
    // In a RH camera with -Z forward, vz should be negative for points in front
    return -abs(vz);
}

float fetch_view_z(float2 uv, float z_near, float z_far)
{
    Texture2D<float> depth_full = sfg_get_texture2Df(sfg_rp_constant0);
    float d = depth_full.SampleLevel(smp_point, uv, 0);
    return depth_to_view_z(d, z_near, z_far);
}

float3 fetch_view_normal(float2 uv, float3x3 world_to_view)
{
    Texture2D<float> norm_enc_full = sfg_get_texture2Df(sfg_rp_constant1);
    float2 enc = norm_enc_full.SampleLevel(smp_linear, uv, 0);

    float3 n_world = oct_decode(enc);
    float3 n_view = mul(world_to_view, n_world);
    return normalize(n_view);
}

// Reconstruct view-space position from uv + view-space z (negative forward)
float3 reconstruct_view_pos(float2 uv, float viewZ, float fx, float fy)
{
    float2 ndc = uv * 2.0f - 1.0f; // [-1,1]
    float vx = ndc.x * (-viewZ) / fx;
    float vy = ndc.y * (-viewZ) / fy;
    return float3(vx, vy, viewZ);
}

// Build an orthonormal basis (T,B) around normal N
void build_tangent_frame(float3 N, out float3 T, out float3 B)
{
    float3 up = (abs(N.z) < 0.999f) ? float3(0,0,1) : float3(0,1,0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

// Distance falloff (tweakable). Keeps nearby occluders stronger.
float falloff(float dist)
{
    // Inverse quad with small constant to avoid singularity
    return 1.0f / (1.0f + 4.0f * dist * dist);
}

// ----------------------------------------------
// Main CS
// ----------------------------------------------
[numthreads(8,8,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    // Half-res pixel
    uint2 pH = DTid.xy;
    ao_params params = sfg_get_cbv<ao_params>(sfg_rp_ubo_index);

    if (pH.x >= params.half_size.x || pH.y >= params.half_size.y)
        return;

    // Map half-res center to full-res UV:
    // half pixel center at (2*h + 1) in full-res pixel space
    float2 uv_full = ( (float2(pH) * 2.0f + 1.0f) * params.inv_full);

    // Fetch center depth/normal and reconstruct view position
    float vzC   = fetch_view_z(uv_full, params.z_near, params.z_far);                   // view-space z (negative forward)
    float3 nC   = fetch_view_normal(uv_full, params.world_to_view_rot);              // view-space normal (unit)
    float3 pC   = reconstruct_view_pos(uv_full, vzC, params.fx, params.fy);      // view-space position

    // Early out if depth invalid (e.g., sky) — optional when tagging sky by depth
    // if (vzC >= -1e-4f) { ao_half_out[pH] = 0.0f; return; }

    // Tangent frame in view-space
    float3 T, B;
    build_tangent_frame(nC, T, B);

    // Per-pixel random rotation (prevents banding)
    // Tile the small noise over full-res UV
    Texture2D noise_tex = sfg_get_texture2D(sfg_rp_constant2);
    float2 noise = noise_tex.SampleLevel(smp_point, uv_full, 0).xy;
    float rot = atan2(noise.y, noise.x) * params.random_rot_strength;

    // Accumulate occlusion over directions and steps
    float ao_accum = 0.0f;

    // Perspective-correct radius: convert world/view radius to a "reasonable" sampling scale.
    // We step in *view-space* along the tangent directions by dist in [0, radius_world].
    // Note: We still *sample* depth via screen projection of those points.
    [loop]
    for (uint d = 0; d < params.num_dirs; ++d)
    {
        float ang = rot + (2.0f * PI) * (d / (float)params.num_dirs);
        float3 dir_vs = normalize(T * cos(ang) + B * sin(ang)); // unit vector on tangent plane

        float dir_occl = 0.0f;

        [loop]
        for (uint s = 1; s <= params.num_steps; ++s)
        {
            float t     = s / (float)params.num_steps;     // 0..1
            float dist  = t * params.radius_world;         // meters (view units)
            float3 qVS  = pC + dir_vs * dist;       // candidate sample point in view space

            // Project qVS to screen (full-res uv) to fetch actual scene depth & normal there
            float ndcX =  ( qVS.x * params.fx / -qVS.z );
            float ndcY =  ( qVS.y * params.fy / -qVS.z );
            float2 uvQ = float2(ndcX, ndcY) * 0.5f + 0.5f;

            // Off-screen? stop marching this direction
            if (uvQ.x <= 0.0f || uvQ.x >= 1.0f || uvQ.y <= 0.0f || uvQ.y >= 1.0f)
                break;

            float vzS = fetch_view_z(uvQ, params.z_near, params.z_far);
            float3 pS = reconstruct_view_pos(uvQ, vzS, params.fx, params.fy);

            // Project (S - P) onto the direction in the tangent plane:
            float height = dot(pS - pC, dir_vs);  // signed "rise" along dir_vs
            float slope  = (height - params.bias) / max(dist, 1e-3f);

            // Simple occlusion when sample rises above horizon (positive slope),
            // weighted by distance falloff. This is a robust, compact proxy.
            if (slope > 0.0f)
            {
                dir_occl += falloff(dist);
            }
        }

        ao_accum += dir_occl;
    }

    // Normalize and shape
    float maxAccum = (float)params.num_dirs * (float)params.num_steps;    // loose upper bound
    float ao = ao_accum / (0.5f * maxAccum);               // heuristic normalization
    ao = saturate(ao * params.intensity);
    ao = pow(ao, params.power);

    // Write half-res AO
    RWTexture2D<float> ao_half_out = sfg_get_rwtexture2D(sfg_rp_constant3);
    ao_half_out[pH] = ao;
}
