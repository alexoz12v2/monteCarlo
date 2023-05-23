struct VS_INPUT
{
	float3 pos : POSITION;
	uint color : COLOR;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
};

float4 convertABGRToFloat4(uint abgrColor)
{
    float4 color;

    // Extract individual color components
    uint alpha = (abgrColor >> 24) & 0xFF;
    uint blue = (abgrColor >> 16) & 0xFF;
    uint green = (abgrColor >> 8) & 0xFF;
    uint red = abgrColor & 0xFF;

    // Normalize the color components to the range [0, 1]
    color.x = red / 255.0;
    color.y = green / 255.0;
    color.z = blue / 255.0;
    color.w = alpha / 255.0;

    return color;
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.pos = float4(input.pos, 1.0);
	output.color = convertABGRToFloat4(input.color);
	return output;
}

