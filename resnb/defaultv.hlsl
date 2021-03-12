struct VSOut {
  float3 normal : Normal; 
  float2 tex : Texture;
  float4 worldPosition : WorldPosition;
  float4 position : SV_Position;
};

cbuffer matrices : register(b0) {
  matrix perspective;
  matrix transform;
  matrix camera;
};

VSOut main(float3 position : Position, float3 normal : Normal, float2 tex : Texture) {
  VSOut vout;
  vout.normal = normal;
  vout.tex = tex;
  vout.worldPosition = mul(float4(position, 1), transform);
  vout.position = mul(mul(vout.worldPosition, camera), perspective);
  return vout;
}
