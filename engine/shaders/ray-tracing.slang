struct PushContants {
    // Compiling with slangc uses col-major format:
    // see https://shader-slang.org/slang/user-guide/a1-01-matrix-layout.html#two-conventions-of-matrix-transform-math
    float4x4 viewMatrix;
    float4x4 inverseViewMatrix;
    float4x4 projectionMatrix;
    float4x4 inverseProjectionMatrix;
};

[[vk::push_constant]] PushContants pcs;

[[vk::binding(0, 1)]] RaytracingAccelerationStructure tlas;
[[vk::binding(1, 1)]] RWTexture2D<float4> image;

struct Payload {
    float3 color;
    float weight;
    int depth;
}

[shader("raygeneration")]
void rayGenerationProgram() {
    float2 pixelIdx = DispatchRaysIndex().xy;
    float2 extent = DispatchRaysDimensions().xy;

    uint rayFlags = 0;

    // Convert pixelIdx to 2d clipCoords
    const float2 clipCoords = pixelIdx / extent * 2.0 - 1.0;

    // Ray direction in view space
    const float4 viewCoords = mul(float4(clipCoords, 1.0, 1.0), pcs.inverseProjectionMatrix);

    RayDesc ray;
    ray.Origin = mul(float4(0.0, 0.0, 0.0, 1.0), pcs.inverseViewMatrix).xyz;
    ray.Direction = mul(float4(normalize(viewCoords.xyz), 0.0), pcs.inverseViewMatrix).xyz;
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    Payload payload;
    payload.color = float3(0, 0, 0);
    payload.weight = 1;
    payload.depth = 0;

    TraceRay(tlas, 0, 0xff, 0, 0, 0, ray, payload);

    // Write payload color to image
    image[int2(pixelIdx)] = float4(payload.color, 1.0);
}

[shader("closesthit")]
void closestHitProgram(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr) {
    // Write red to closest hit rays
    payload.color = float3(1, 0, 0);
}

[shader("miss")]
void missProgram(inout Payload payload) {
    // Write gray to missed rays
    payload.color = float3(0.5, 0.5, 0.5);
}