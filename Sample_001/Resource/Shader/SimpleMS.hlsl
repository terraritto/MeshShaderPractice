struct VertexInput
{
    float3 Position;
    float4 Color;
};
struct VertexOutput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
};

struct PrimitiveOutput
{
    uint PrimitiveId : INDEX0;
};

StructuredBuffer<VertexInput>   Vertices    : register(t0);
StructuredBuffer<uint3>         Indices     : register(t1);

[numthreads(32, 1, 1)]
[outputtopology("triangle")]
void main
(
    uint groupIndex : SV_GroupIndex,
    out vertices VertexOutput verts[3],
    out indices uint3 tris[1],
    out primitives PrimitiveOutput prims[1]
)
{
    SetMeshOutputCounts(3, 1);

    if (groupIndex < 1)
    {
        tris[groupIndex] = Indices[groupIndex];
        prims[groupIndex].PrimitiveId = groupIndex;
    }

    if (groupIndex < 3)
    {
        VertexOutput vout;
        vout.Position   = float4(Vertices[groupIndex].Position, 1.0f);
        vout.Color      = Vertices[groupIndex].Color;

        verts[groupIndex] = vout;
    }
}