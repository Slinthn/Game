struct VSOut {
  float3 normal : Normal; 
  float2 tex : Texture;
  float4 position : SV_Position;
};

cbuffer VS_CONSTANT_BUFFER : register(b0) {
  matrix perspective;
  matrix transform;
  matrix camera;
};

VSOut main(float3 position : Position, float3 normal : Normal, float2 tex : Texture) {
  VSOut vout;
  vout.position = mul(mul(mul(float4(position, 1), transform), camera), perspective);
  vout.normal = normal;
  vout.tex = tex;
  return vout;
}
