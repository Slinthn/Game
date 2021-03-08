#include <stdint.h>
typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;

typedef float r32;
typedef double r64;

typedef s32 b32;

#ifdef SLINGAME_DEBUG
#define assert(x) if (!(x)) *((u8 *)0) = 0;
#else
#define assert(x)
#endif

typedef r32 MATRIX4F[16];
typedef r32 VECTOR3F[3];
typedef r32 VECTOR2F[2];

typedef struct {
  IDXGISwapChain *swapchain;
  ID3D11Device *device;
  ID3D11DeviceContext *context;
  ID3D11RenderTargetView *renderView;
  ID3D11DepthStencilView *depthStencilView;
} DX;

typedef struct {
  MATRIX4F perspective;
  MATRIX4F transform;
  MATRIX4F camera;
} DXCBUF0;

typedef struct {
  void *memory;
  u32 size;
} FILE;

typedef struct {
  FILE file;
  u32 vertexCount;
  u32 faceCount;
  r32 *vertices;
  u32 *faces;
} MODEL;

typedef struct {
  u32 a;
} DXMODEL;

typedef struct {
  FILE file;
  u32 width;
  u32 height;
  u8 *pixels;
} TEXTURE;

typedef struct {
  ID3D11VertexShader *vShader;
  ID3D11PixelShader *pShader;
} DXSHADER;

typedef struct {
  VECTOR2F l;
  VECTOR2F r;
} PLAYERINPUT;

typedef struct {
  VECTOR3F pos;
  VECTOR3F rot;
} PLAYER;

typedef struct {
  ID3D11Buffer *cbuffer;
  DXSHADER shader;
  ID3D11SamplerState *samplerState;
  TEXTURE texture;
  ID3D11ShaderResourceView *textureView;
  DXCBUF0 cbuf;
  MODEL model;
  PLAYER player;
} GAME;
