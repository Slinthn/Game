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

#define sizeofarray(array) (sizeof(array) / sizeof((array)[0]))

#ifdef SLINGAME_DEBUG
#define assert(x) if (!(x)) *((u8 *)0) = 0;
#else
#define assert(x) ;
#endif

typedef r32 MATRIX4F[16];

typedef struct {
  r32 x, y;
} VECTOR2F;

typedef struct {
  r32 x, y, z;
} VECTOR3F;

typedef struct {
  r32 x, y, z, w;
} VECTOR4F;

typedef struct {
  VECTOR2F lStick;
  VECTOR2F rStick;
  b32 buttons;
  u8 battery;
  VECTOR3F gyro;
  VECTOR3F accel;
} CONTROLLERINPUT;

typedef struct {
  u8 rMotor, lMotor;
  u8 r, g, b;
} CONTROLLEROUTPUT;

#define CONTROLLER_DEADZONE 0.1f

#define CONTROLLER_BUTTON_A 0x1
#define CONTROLLER_BUTTON_B 0x2
#define CONTROLLER_BUTTON_X 0x4
#define CONTROLLER_BUTTON_Y 0x8

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
  ID3D11Buffer *indexBuffer;
  ID3D11Buffer *vertexBuffer;
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
  b32 down;
  u32 transitions;
} BUTTON;

#define MAX_BUTTONS 1
typedef struct {
  VECTOR2F left;
  VECTOR2F right;
  union {
    BUTTON buttons[MAX_BUTTONS];
    struct {
      BUTTON action0;
    };
  } button;
} KEYBOARDINPUT;

typedef struct {
  VECTOR3F position;
  VECTOR3F rotation;
} PLAYER;

typedef struct {
  u32 type;
  MODEL *model;
  VECTOR3F position;
  VECTOR3F rotation;  
} ENTITY;

typedef struct {
  VECTOR4F surroundlights[10];
} DXCBUF1;

typedef struct {
  ID3D11Buffer *cbuffer[2];
  DXSHADER shader;
  ID3D11SamplerState *samplerState;
  TEXTURE texture;
  ID3D11ShaderResourceView *textureView;
  DXCBUF0 cbuf0;
  DXCBUF1 cbuf1;
  MODEL models[2];
  DXMODEL dxmodels[2];
  PLAYER player;
  ENTITY entities[10];
} GAME;
