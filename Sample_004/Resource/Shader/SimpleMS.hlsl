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

// Payload
struct Payload
{
    uint InstanceIndices[AS_GROUP_SIZE];
	uint MeshletIndices[AS_GROUP_SIZE];
    uint DebugColor[AS_GROUP_SIZE];
};

// Meshlet
struct VertexInput
{
    float3 Position;
};

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

StructuredBuffer<VertexInput>   Vertices            : register(t0);
StructuredBuffer<Meshlet>       Meshlets            : register(t1);
StructuredBuffer<uint>          VertexIndices       : register(t2);
StructuredBuffer<uint>          TriangleIndices     : register(t3);
StructuredBuffer<Instance>      Instances           : register(t4);

struct VertexOutput
{
    float4 Position : SV_POSITION;
    float3 Color    : COLOR;
};

[outputtopology("triangle")]
[numthreads(128, 1, 1)]
void main
(
    uint groupThreadIndex : SV_GroupThreadID,
    uint groupIndex : SV_GroupID,
    in payload Payload payload,
    out vertices VertexOutput vertices[64],
    out indices uint3 triangles[128]
)
{
    // unpack index
    uint instanceIndex = payload.InstanceIndices[groupIndex];
    uint meshletIndex = payload.MeshletIndices[groupIndex];
    
    // unpack Color
    float3 debugColor = payload.DebugColor[groupIndex] == 1 ? float3(1.0f, 0.0f, 0.0f) : float3(0.0f, 0.0f, 1.0f);

    // get meshlet from index
    Meshlet meshlet = Meshlets[meshletIndex];

    // set output count
    SetMeshOutputCounts(meshlet.VertexCount, meshlet.TriangleCount);

    // triangle index
    if (groupThreadIndex < meshlet.TriangleCount)
    {
        uint packed = TriangleIndices[meshlet.TriangleOffset + groupThreadIndex];
        uint vIndex0 = (packed >>  0) & 0xFF;
        uint vIndex1 = (packed >>  8) & 0xFF;
        uint vIndex2 = (packed >> 16) & 0xFF;
        triangles[groupThreadIndex] = uint3(vIndex0, vIndex1, vIndex2);
    }

    // vertex transform
    if (groupThreadIndex < meshlet.VertexCount)
    {
        uint vertexIndex = meshlet.VertexOffset + groupThreadIndex;
        vertexIndex = VertexIndices[vertexIndex];

        float4x4 mvp = mul(Scene.MVP, Instances[instanceIndex].Mat);

        VertexOutput vout;
        vout.Position   = mul(mvp, float4(Vertices[vertexIndex].Position, 1.0f));   
   
        if (Scene.DebugFrustum == 0.0f)
        {
            vout.Color = float3(float(groupIndex & 1), float(groupIndex & 3) / 4, float(groupIndex & 7) / 8);        
        }
        else
        {
            vout.Color = debugColor;
        }

        vertices[groupThreadIndex] = vout;
    }
}