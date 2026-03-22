// Camera
struct CameraProperties
{
    float4x4 MVP;
};

ConstantBuffer<CameraProperties> Camera : register(b0);

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
};

StructuredBuffer<VertexInput>   Vertices            : register(t0);
StructuredBuffer<Meshlet>       Meshlets            : register(t1);
StructuredBuffer<uint>          VertexIndices       : register(t2);
StructuredBuffer<uint>          TriangleIndices     : register(t3);

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
    out vertices VertexOutput vertices[64],
    out indices uint3 triangles[128]
)
{
    // get meshlet from index
    Meshlet meshlet = Meshlets[groupIndex];

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

        VertexOutput vout;
        vout.Position   = mul(Camera.MVP, float4(Vertices[vertexIndex].Position, 1.0f));   
        vout.Color      = float3(float(groupIndex & 1), float(groupIndex & 3) / 4, float(groupIndex & 7) / 8);        
        vertices[groupThreadIndex] = vout;
    }
}