cbuffer VS_CONSTANT_BUFFER : register(b0)
{
   float pixel_scale;
}; // don't actually need this yet lol

struct VOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
	float2 test_pos : VPOS;
};

Texture2D test : register(t0);
SamplerState default_sampler : register(s0);

VOut vs_main(float4 position : POSITION, float4 color : COLOR)
{
    VOut output;

    output.position = position;
    output.color = color;

    return output;
}


float4 ps_main(float4 position : SV_POSITION, float4 color : COLOR) : SV_TARGET
{
	float u = position.x / 800.0f;
	float v = 1.0f - (position.y / 600.0f);

	//return float4(u, v, 0.0f, 1.0f);

    return test.Sample(default_sampler, float2(u, v));
	//return color;
}