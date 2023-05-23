
struct FSInput {
	float4 color : COLOR0;
};

float4 main(FSInput input) : SV_TARGET {
	return input.color;
}
