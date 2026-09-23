#define AS_GROUP_SIZE 32  // thread group size

struct ConstantProperties
{
    uint InstanceCount;
    uint MeshletCount;
};

struct SceneProperties
{
    float4x4 MVP;
    float4 Planes[6];
    float4 DebugPlanes[6];
    float DebugFrustum;
};

ConstantBuffer<ConstantProperties> Constant : register(b0);
ConstantBuffer<SceneProperties> Scene : register(b1);

struct Meshlet
{
    uint VertexOffset;
    uint VertexCount;
    uint TriangleOffset;
    uint TriangleCount;
    uint NormalCone;
    float4 BoundingSphere;
};

struct Instance
{
    float4x4 Mat;
};

StructuredBuffer<Meshlet>       Meshlets            : register(t1);
StructuredBuffer<Instance>      Instances           : register(t4);

struct Payload
{
    uint InstanceIndices[AS_GROUP_SIZE];
	uint MeshletIndices[AS_GROUP_SIZE];
    uint DebugColor[AS_GROUP_SIZE];
};

groupshared Payload sPayload;

float4 TransformSphere(float4 sphere, float4x4 world)
{
    // convert world coordinate.
    float4 value = mul(world, float4(sphere.xyz, 1.0f));

    // calculate Length Square per axis
    float sx = dot(world._11_12_13, world._11_12_13);
    float sy = dot(world._21_22_23, world._21_22_23);
    float sz = dot(world._31_32_33, world._31_32_33);

    // adopt max length sq
    float scale = sqrt(max(sx, max(sy, sz)));

    return float4(value.xyz, sphere.w * scale);
}

bool Contains(float4 planes[6], float4 sphere)
{
    float4 center = float4(sphere.xyz, 1.0f);

    for (int i = 0; i < 6; ++i)
    {
        if (dot(center, planes[i]) < -sphere.w)
        {
            return false;
        }
    }

    return true;
}

bool IsVisible(Meshlet meshlet, float4 planes[6], float4x4 world, float4x4 viewProj)
{
    float4 sphere = TransformSphere(meshlet.BoundingSphere, world);

    // frustum culling
    if (!Contains(planes, sphere))
    {
        return false;
    }

    return true;
}

[numthreads(AS_GROUP_SIZE, 1, 1)]
void main
(
    uint groupThreadIndex : SV_GroupThreadID,
    uint dispatchThreadIndex : SV_DispatchThreadID,
    uint groupIndex : SV_GroupID
)
{
    bool visible = false;
    bool isInside = false;

    // calculate index
    uint instanceIndex = dispatchThreadIndex / Constant.MeshletCount;
    uint meshletIndex  = dispatchThreadIndex % Constant.MeshletCount;

    // if not exceed count, set index.
    if ((instanceIndex < Constant.InstanceCount) && (meshletIndex < Constant.MeshletCount))
    {   
        Meshlet meshlet = Meshlets[meshletIndex];
        Instance instance = Instances[instanceIndex];

        if (Scene.DebugFrustum < 0.5f)
        {
            visible = IsVisible(meshlet, Scene.Planes, instance.Mat, Scene.MVP);
        }
        else
        {
            // debug
            isInside = IsVisible(meshlet, Scene.DebugPlanes, instance.Mat, Scene.MVP);
            visible = true;
        }
    }

    if (visible)
    {
        uint index = WavePrefixCountBits(visible);
        sPayload.InstanceIndices[index] = instanceIndex;
        sPayload.MeshletIndices[index] = meshletIndex;
        sPayload.DebugColor[index] = isInside ? 1 : 2;
    }

    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, sPayload);
}