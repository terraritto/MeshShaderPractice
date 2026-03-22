struct VertexOutput
{
    float4 Position : SV_POSITION;
    float3 Color    : COLOR;
};

// Output Color Only.
float4 main(VertexOutput input) : SV_TARGET
{
    return float4(input.Color, 1.0f);
}
