Texture2D tex : register(t0);

SamplerState splr : register(s0);

cbuffer lighting : register(b1) {
  float4 surroundlights[10];
};

float4 main(float3 normal : Normal, float2 texCoord : Texture, float4 worldPosition : WorldPosition) : SV_Target {
  float lightPower = 0;
  for (uint i = 0; i < 10; i++) {
    float dx = surroundlights[i].x - worldPosition.x;
    float dy = surroundlights[i].y - worldPosition.y;
    float dz = surroundlights[i].z - worldPosition.z;
    float dist = sqrt(dx*dx + dy*dy + dz*dz); // TODO(slin): remove sqrt

    float min = 10;
    if (dist < min)
      lightPower += (min - dist) / min;
  }

  lightPower = max(min(lightPower, 1), 0.2f);  
  return lightPower*tex.Sample(splr, texCoord);
}
