
struct FSInput {
	uint color : COLOR0;
};

float4 main(FSInput input) : SV_TARGET {
	float4 color = float4(
		((input.color >> 16) & 0xff) / 255.0f,
		((input.color >> 8) & 0xff) / 255.0f,
		(input.color & 0xff) / 255.0f,
		((input.color >> 24) & 0xff) / 255.0f
	);
	return color;
}
