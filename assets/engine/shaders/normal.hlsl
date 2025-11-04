float2 oct_encode(float3 n) {
    n /= (abs(n.x) + abs(n.y) + abs(n.z) + 1e-8);
    float2 enc = (n.z >= 0.0) ? n.xy : ((1.0 - abs(n.yx)) * (float2(n.x >= 0 ? 1 : -1, n.y >= 0 ? 1 : -1)));
    return enc * 0.5 + 0.5;
}

float3 oct_decode(float2 e) {
    // [0,1] -> [-1,1]
    float2 f = e * 2.0 - 1.0;
    float3 n = float3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = saturate(-n.z);
    // fold the lower hemisphere
    n.x += (n.x >= 0 ? -t : t);
    n.y += (n.y >= 0 ? -t : t);
    return normalize(n);
}

