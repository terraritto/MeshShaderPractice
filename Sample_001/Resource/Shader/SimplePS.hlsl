struct VertexOutput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
};

// Output Color Only.
float4 main(VertexOutput input) : SV_TARGET
{
    return input.Color;
}
