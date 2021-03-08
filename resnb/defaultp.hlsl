Texture2D tex : register(t0);

SamplerState splr : register(s0);

float4 main(float3 normal : Normal, float2 texCoord : Texture) : SV_Target {
  return tex.Sample(splr, texCoord);//*float4(normal, 1);
}
