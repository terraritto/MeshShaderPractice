#define AS_GROUP_SIZE 32  // thread group size

struct ConstantProperties
{
    uint InstanceCount;
    uint MeshletCount;
    float Contribution;
};

struct SceneProperties
{
    float4x4 View;
    float4x4 Proj;
    float3 CameraPosition;
    float4 Planes[6];
    float3 DebugCameraPosition;
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
};

struct PrimitiveOutput
{
    float3 Color   : COLOR0;
    bool   Culling : SV_CullPrimitive;
};

// Back Face Culling and Zero Area Culling
bool IsBackFaceOrZeroArea(float3 worldPos[3], float3 cameraPosition)
{
    float3 viewDirection = normalize(cameraPosition - worldPos[0]);

    float3 a = worldPos[1] - worldPos[0];
    float3 b = worldPos[2] - worldPos[0];
    float3 n = cross(a, b);
    return dot(n, viewDirection) <= 0.0f;
}

[outputtopology("triangle")]
[numthreads(128, 1, 1)]
void main
(
    uint groupThreadIndex : SV_GroupThreadID,
    uint groupIndex : SV_GroupID,
    in payload Payload payload,
    out vertices VertexOutput vertices[64],
    out indices uint3 triangles[128],
    out primitives PrimitiveOutput primitives[128]
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

        uint3 Index = uint3(vIndex0, vIndex1, vIndex2);
        triangles[groupThreadIndex] = Index;

        // for Culling
        float3 posW[3];

        // vertex transform
        for (uint j = 0; j < 3; j++)
        {
            uint idx = Index[j];
            
            // Check Array Region.
            if (idx >= meshlet.VertexCount) { continue; }

            // Vertex
            uint vertexIndex = meshlet.VertexOffset + idx;
            vertexIndex = VertexIndices[vertexIndex];

            float4 localPosition = float4(Vertices[vertexIndex].Position, 1.0f);
            float4 worldPosition = mul(Instances[instanceIndex].Mat, localPosition);
            float4 projectionPosition = mul(Scene.Proj, mul(Scene.View, worldPosition));

            VertexOutput vout;
            vout.Position = projectionPosition;
            vertices[idx] = vout;

            // set culling parameter
            posW[j] = worldPosition.xyz;
        }

        // culling calculation
        bool culled = false;
        if (Scene.DebugFrustum == 0.0f)
        {
            culled |= IsBackFaceOrZeroArea(posW, Scene.CameraPosition);
        }
        else
        {
            culled |= IsBackFaceOrZeroArea(posW, Scene.DebugCameraPosition);
        }

        // Primitives
        PrimitiveOutput primOut = (PrimitiveOutput)0;
        if (Scene.DebugFrustum == 0.0f)
        {
            primOut.Color = float3(float(groupIndex & 1), float(groupIndex & 3) / 4, float(groupIndex & 7) / 8);        
        }
        else
        {
            primOut.Color = (payload.DebugColor[groupIndex] == 1 && culled) ? float3(0.0f, 1.0f, 0.0f) : debugColor;
            culled = false;
        }

        primOut.Culling = culled;
        primitives[groupThreadIndex] = primOut;
    }
}