// taki shader nie potrzebuje danych wejœciowych, dzia³a na podstawie kolejnych ID wierzcho³ków
// wymaga za to Shader Model 4.0 - zmiana ustawieñ domyœlnych
float4 main( uint id : SV_VertexID, out float2 coords : TEXCOORD ) : SV_POSITION
{
	coords = float2( // wspó³rzêdne tekstury zamieni¹ siê w docelowy kolor
		id & 1 ? 0 : 1,  // x: 0 | 1 | 0 | 1
		id & 2 ? 1 : 0); // y: 1 | 1 | 0 | 0
	return float4(coords, 0, 1);
}