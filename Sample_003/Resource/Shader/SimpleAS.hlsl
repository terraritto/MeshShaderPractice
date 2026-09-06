#define AS_GROUP_SIZE 32  // thread group size

struct SceneProperties
{
    float4x4 MVP;
    uint InstanceCount;
    uint MeshletCount;
};

ConstantBuffer<SceneProperties> Scene : register(b0);

struct Payload
{
    uint InstanceIndices[AS_GROUP_SIZE];
	uint MeshletIndices[AS_GROUP_SIZE];
};

groupshared Payload sPayload;

[numthreads(AS_GROUP_SIZE, 1, 1)]
void main
(
    uint groupThreadIndex : SV_GroupThreadID,
    uint dispatchThreadIndex : SV_DispatchThreadID
)
{
    bool visible = false;

    // calculate index
    uint instanceIndex = dispatchThreadIndex / Scene.MeshletCount;
    uint meshletIndex  = dispatchThreadIndex % Scene.MeshletCount;

    // if not exceed count, set index.
    if ((instanceIndex < Scene.InstanceCount) && (meshletIndex < Scene.MeshletCount))
    {
        visible = true;
        sPayload.InstanceIndices[groupThreadIndex] = instanceIndex;
        sPayload.MeshletIndices[groupThreadIndex] = meshletIndex;
    }

    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, sPayload);
}