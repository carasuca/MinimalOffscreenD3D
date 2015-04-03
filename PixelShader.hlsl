float4 main(float2 coords : TEXCOORD) : SV_TARGET
{
	return float4(coords, 0, 1);
}