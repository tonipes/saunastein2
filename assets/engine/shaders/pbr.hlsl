//------------------------------------------------------------------------------
// PBR Core Functions (Cook-Torrance BRDF)
//------------------------------------------------------------------------------

// Global constant for PBR calculations
static const float PI = 3.14159265359;

// D: Distribution function - Trowbridge-Reitz GGX
float distribution_ggx(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

// G: Geometry function - Schlick approximation of GGX
float geometry_schlick_ggx(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float geometry_smith(float NdotV, float NdotL, float roughness)
{
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);
    return ggx1 * ggx2;
}

// F: Fresnel function - Schlick approximation
float3 fresnel_schlick(float cosTheta, float3 F0)
{
    // F0 is the base reflectivity at normal incidence (0 degrees)
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}



//------------------------------------------------------------------------------
// PBR Lighting Function (for a single light source)
//------------------------------------------------------------------------------
float3 calculate_pbr(
    float3 V, // View direction (to camera)
    float3 N, // World Normal
    float3 L, // Light direction (from light)
    float3 albedo,
    float ao,
    float roughness,
    float metallic,
    float3 radiance, // Light color/intensity
    float NdotL
)
{
    // 1. Calculate base reflectivity F0
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);

    // 2. Halfway vector H
    float3 H = normalize(V + L);
    float NdotH = saturate(dot(N, H));
    float NdotV = saturate(dot(N, V));

    // 3. PBR Terms
    float D = distribution_ggx(NdotH, roughness); 
    float G = geometry_smith(NdotV, NdotL, roughness);
    float3 F = fresnel_schlick(NdotH, F0);
    
    // 4. Specular term (Microfacet BRDF)
    float3 nominator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.001; // Avoid divide by zero
    float3 specular = nominator / denominator;
    
    // 5. Diffuse term (Energy Conservation)
    float3 kS = F; // Fresnel is the amount of light reflected (specular)
    float3 kD = (float3(1.0, 1.0, 1.0) - kS) * (1.0 - metallic); // Remaining light is diffuse

    // 6. Final Radiance (BRDF * L dot N * Radiance)
    float3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    
    // 7. Apply Ambient Occlusion (AO) to the diffuse term only
    // Note: A more advanced approach applies AO to both diffuse and specular IBL, 
    // but for simple point lights, this approximation is common.
    float3 final_light = Lo * ao;

    return final_light;
}
