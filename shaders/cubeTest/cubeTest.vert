struct VS_INPUT
{
	float3 pos : POSITION;
	float4 color : COLOR;
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
    color.x = float(red) / 255;
    color.y = float(green) / 255;
    color.z = float(blue) / 255;
    color.w = float(alpha) / 255;

    return color;
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.pos = float4(input.pos, 1.0);
	output.color = input.color / 255;// convertABGRToFloat4(input.color);
	return output;
}

