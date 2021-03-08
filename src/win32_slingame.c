#pragma warning(push, 0)
#include <windows.h>
#include <hidusage.h>
#include <d3d11.h>
#pragma warning(pop)

int sprintf_s(char *, size_t, char *, ...);

#include "types.h"
#include "math.c"

#ifdef SLINGAME_DEBUG
HANDLE debugFile;
void initDebugLog(void) {
  debugFile = CreateFileA("debug.log", FILE_APPEND_DATA, 0, 0, CREATE_ALWAYS, 0, 0);
}
void debugLog(char *msg) {
  DWORD written;
  WriteFile(debugFile, msg, strlen(msg), &written, 0);
  WriteFile(debugFile, "\n", 1, &written, 0);
}
#else
#define initDebugLog() ;
#define debugLog(x) ;
#endif

static b32 running;

FILE readFile(char *filename) {
  FILE result = {0};
  
  HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if (!fileHandle)
    return result;

  result.size = GetFileSize(fileHandle, 0);
  result.memory = VirtualAlloc(0, result.size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

  DWORD read;
  ReadFile(fileHandle, result.memory, result.size, &read, 0);
  CloseHandle(fileHandle);
  return result;
}

void freeFile(FILE *file) {  
  VirtualFree(file->memory, 0, MEM_RELEASE);
  *file = (FILE){0};
}

void writeFile(char *filename, u8 *data, u32 bytes) {
  HANDLE fileHandle;
  DWORD written;
  
  fileHandle = CreateFileA(filename, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
  if (!fileHandle)
    return;

  WriteFile(fileHandle, data, bytes, &written, 0);
  CloseHandle(fileHandle);
}

LRESULT CALLBACK win32WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;
  switch (message) {
  case WM_CREATE: {
    running = 1;
  } break;
    
  case WM_DESTROY:
  case WM_CLOSE: {
    running = 0;
  } break;

  default: {
    result = DefWindowProc(window, message, wParam, lParam);;
  } break;
  }
  
  return result;
}

HWND win32CreateWindow(HINSTANCE instance) {
  WNDCLASSA wc = {0};
  wc.lpfnWndProc = win32WindowProc;
  wc.hInstance = instance;
  wc.lpszClassName = "SlinGameWindowClass";
  RegisterClass(&wc);

  debugLog("Registered window class.");
  
  return CreateWindowA(wc.lpszClassName, "Slin Game", WS_OVERLAPPED|WS_MINIMIZEBOX|WS_CAPTION|WS_SYSMENU|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, 0, 0, instance, 0);
}

void win32InitRawInput(HWND window) {
  RAWINPUTDEVICE ri[2];
  ri[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
  ri[0].usUsage = HID_USAGE_GENERIC_MOUSE;
  ri[0].dwFlags = 0;
  ri[0].hwndTarget = window;
  ri[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
  ri[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
  ri[1].dwFlags = 0;
  ri[1].hwndTarget = window;
  
  RegisterRawInputDevices(ri, 2, sizeof(RAWINPUTDEVICE));
}

void win32HandleRawInput(MSG *message, PLAYERINPUT *input) {
  static b32 mouseCapture = 0;
  
  u32 size = sizeof(RAWINPUT);
  RAWINPUT ri;
  GetRawInputData((HRAWINPUT)message->lParam, RID_INPUT, &ri, &size, sizeof(RAWINPUTHEADER));
  if (ri.header.dwType == RIM_TYPEKEYBOARD) {
    u16 key = ri.data.keyboard.VKey;
    b32 down = !(ri.data.keyboard.Flags & RI_KEY_BREAK);
    switch (key) {
    case 'W': {
      input->l[1] = (r32)down;
    } break;
    case 'A': {
      input->l[0] = (r32)-down;
    } break;
    case 'S': {
      input->l[1] = (r32)-down;
    } break;
    case 'D': {
      input->l[0] = (r32)down;
    } break;
    case VK_ESCAPE: {
      if (mouseCapture) {
        mouseCapture = 0;
        ShowCursor(1);
        ClipCursor(0);
      }
    } break;
    }
  } else if (ri.header.dwType == RIM_TYPEMOUSE) {
    b32 lDown = ri.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN;
    if (lDown && !mouseCapture) {
      RECT rect;
      GetClientRect(message->hwnd, &rect);
      POINT topLeft = {rect.left, rect.top};
      POINT bottomRight = {rect.right, rect.bottom};
      ClientToScreen(message->hwnd, &topLeft);
      ClientToScreen(message->hwnd, &bottomRight);
      rect.left = topLeft.x;
      rect.top = topLeft.y;
      rect.right = bottomRight.x;
      rect.bottom = bottomRight.y;

      POINT cursor;
      GetCursorPos(&cursor);
      if (cursor.x > rect.left && cursor.x < rect.right && cursor.y < rect.bottom && cursor.y > rect.top) {
        if (ShowCursor(0) < 0) {
          mouseCapture = 1;
          ClipCursor(&rect);
        }
      }
    }

    if (mouseCapture) {
      input->r[0] = (r32)ri.data.mouse.lLastX;
      input->r[1] = (r32)ri.data.mouse.lLastY;
    }
  }
}

ID3D11Texture2D *createTexture(DX *dx, u32 width, u32 height, void *memory, u32 format, u32 bind) {
  D3D11_TEXTURE2D_DESC desc = {0};
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = format;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = bind;
  desc.CPUAccessFlags = 0;
  desc.MiscFlags = 0;

  ID3D11Texture2D *texture;
  if (memory == 0) {
    dx->device->lpVtbl->CreateTexture2D(dx->device, &desc, 0, &texture);
    return texture;
  }

  D3D11_SUBRESOURCE_DATA sd = {0};
  sd.pSysMem = memory;
  sd.SysMemPitch = width*sizeof(u8)*4;
  
  dx->device->lpVtbl->CreateTexture2D(dx->device, &desc, &sd, &texture);
  return texture;
}

ID3D11ShaderResourceView *createTextureView(DX *dx, ID3D11Texture2D *texture) {
  D3D11_SHADER_RESOURCE_VIEW_DESC srd = {0};
  srd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  srd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srd.Texture2D.MostDetailedMip = 0;
  srd.Texture2D.MipLevels = 1;

  ID3D11ShaderResourceView *textureView;
  dx->device->lpVtbl->CreateShaderResourceView(dx->device, (ID3D11Resource *)texture, &srd, &textureView);
  return textureView;
}

ID3D11SamplerState *createSamplerState(DX *dx) {
  ID3D11SamplerState *samplerState;
  D3D11_SAMPLER_DESC sd = {0};
  sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  dx->device->lpVtbl->CreateSamplerState(dx->device, &sd, &samplerState);
  return samplerState;
}

DX win32InitD3D11(HWND window) {
  DX result = {0};
  HRESULT hresult;

  RECT rect;
  GetWindowRect(window, &rect);
  u32 width = rect.right - rect.left;
  u32 height = rect.bottom - rect.top;
  
  // NOTE(slin): initialisation
  DXGI_SWAP_CHAIN_DESC scd = {0};
  scd.BufferDesc.Width = width;
  scd.BufferDesc.Height = height;
  scd.BufferDesc.RefreshRate.Numerator = 0;
  scd.BufferDesc.RefreshRate.Denominator = 0;
  scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  scd.SampleDesc.Count = 1;
  scd.SampleDesc.Quality = 0;
  scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  scd.BufferCount = 1;
  scd.OutputWindow = window;
  scd.Windowed = 1;
  scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  scd.Flags = 0;
  
  hresult = D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_SINGLETHREADED|D3D11_CREATE_DEVICE_DEBUG, (D3D_FEATURE_LEVEL[1]){D3D_FEATURE_LEVEL_11_0}, 1, D3D11_SDK_VERSION, &scd, &result.swapchain, &result.device, 0, &result.context);
  debugLog("D3D11: Created device and swap chain.");
  assert(hresult == S_OK);

  // NOTE(slin): viewport
  D3D11_VIEWPORT vp;
  vp.Width = (r32)width;
  vp.Height = (r32)height;
  vp.MinDepth = 0;
  vp.MaxDepth = 1;
  vp.TopLeftX = 0;
  vp.TopLeftY = 0;
  result.context->lpVtbl->RSSetViewports(result.context, 1, &vp);
  result.context->lpVtbl->IASetPrimitiveTopology(result.context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  debugLog("D3D11: Created viewport.");
  
  // NOTE(slin): render buffers
  IID id = {3700319219, 53547, 18770, {180, 123, 94, 69, 2, 106, 134, 45}};

  ID3D11Resource *backbuffer;
  result.swapchain->lpVtbl->GetBuffer(result.swapchain, 0, &id, &backbuffer);
  result.device->lpVtbl->CreateRenderTargetView(result.device, backbuffer, 0, &result.renderView);
  backbuffer->lpVtbl->Release(backbuffer);
  debugLog("D3D11: Created render buffer view.");

  ID3D11Texture2D *depthTexture = createTexture(&result, width, height, 0, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL);
  result.device->lpVtbl->CreateDepthStencilView(result.device, (ID3D11Resource *)depthTexture, 0, &result.depthStencilView);
  debugLog("D3D11: Created depth and stencil buffer.");

  result.context->lpVtbl->OMSetRenderTargets(result.context, 1, &result.renderView, result.depthStencilView);
  debugLog("D3D11: Set render targets.");
  return result;
}

DXSHADER createShader(DX *dx, char *vShaderFilename, char *pShaderFilename) {
  DXSHADER result = {0};

  FILE vShaderFile = readFile(vShaderFilename);
  FILE pShaderFile = readFile(pShaderFilename);
  dx->device->lpVtbl->CreateVertexShader(dx->device, vShaderFile.memory, vShaderFile.size, 0, &result.vShader);
  dx->device->lpVtbl->CreatePixelShader(dx->device, pShaderFile.memory, pShaderFile.size, 0, &result.pShader);

  ID3D11InputLayout *inputLayout;
  D3D11_INPUT_ELEMENT_DESC ied[] =
    {
     {"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
     {"Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 3*4, D3D11_INPUT_PER_VERTEX_DATA, 0},
     {"Texture", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 3*4 + 3*4, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

  dx->device->lpVtbl->CreateInputLayout(dx->device, ied, 3, vShaderFile.memory, vShaderFile.size, &inputLayout);
  dx->context->lpVtbl->IASetInputLayout(dx->context, inputLayout);

  freeFile(&vShaderFile);
  freeFile(&pShaderFile);
  return result;
}

ID3D11Buffer *createConstantBuffer(DX *dx, void *memory, u32 size) {
  D3D11_BUFFER_DESC cbd = {0};
  cbd.ByteWidth = size;
  cbd.Usage = D3D11_USAGE_DYNAMIC;
  cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  cbd.MiscFlags = 0;
  cbd.StructureByteStride = 0;
  
  D3D11_SUBRESOURCE_DATA csd = {0};
  csd.pSysMem = memory;

  ID3D11Buffer *constBuffer;
  dx->device->lpVtbl->CreateBuffer(dx->device, &cbd, &csd, &constBuffer);
  return constBuffer;
}

void updateConstantBuffer(DX *dx, ID3D11Buffer *cbuffer, void *memory, u32 size) {
  D3D11_MAPPED_SUBRESOURCE msub;
  dx->context->lpVtbl->Map(dx->context, (ID3D11Resource *)cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msub);
  CopyMemory(msub.pData, memory, size);
  dx->context->lpVtbl->Unmap(dx->context, (ID3D11Resource *)cbuffer, 0);    
}

void createModel(DX *dx, MODEL *model) {
  D3D11_BUFFER_DESC ibd = {0};
  ibd.ByteWidth = model->faceCount*3*sizeof(u32);
  ibd.Usage = D3D11_USAGE_IMMUTABLE;
  ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
  ibd.CPUAccessFlags = 0;
  ibd.MiscFlags = 0;
  ibd.StructureByteStride = sizeof(u32);

  D3D11_SUBRESOURCE_DATA isd = {0};
  isd.pSysMem = model->faces;

  ID3D11Buffer *indexBuffer;
  dx->device->lpVtbl->CreateBuffer(dx->device, &ibd, &isd, &indexBuffer);
  dx->context->lpVtbl->IASetIndexBuffer(dx->context, indexBuffer, DXGI_FORMAT_R32_UINT, 0);

  D3D11_BUFFER_DESC vbd = {0};
  vbd.ByteWidth = model->vertexCount*8*sizeof(r32);
  vbd.Usage = D3D11_USAGE_IMMUTABLE;
  vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  vbd.CPUAccessFlags = 0;
  vbd.MiscFlags = 0;
  vbd.StructureByteStride = 8*sizeof(r32);
    
  D3D11_SUBRESOURCE_DATA vsd = {0};
  vsd.pSysMem = model->vertices;

  ID3D11Buffer *vertexBuffer;
  dx->device->lpVtbl->CreateBuffer(dx->device, &vbd, &vsd, &vertexBuffer);
  dx->context->lpVtbl->IASetVertexBuffers(dx->context, 0, 1, &vertexBuffer, (u32[1]){8*sizeof(r32)}, (u32[1]){0});
}

MODEL loadBPLY(char *filename) {
  MODEL result = {0};
  result.file = readFile(filename);
  result.vertexCount = ((u32 *)result.file.memory)[0];
  result.faceCount = ((u32 *)result.file.memory)[1];
  result.vertices = (r32 *)result.file.memory + 2;
  result.faces = (u32 *)result.file.memory + 2 + result.vertexCount*8;
  return result;
}

TEXTURE loadBBMP(char *filename) {
  TEXTURE result = {0};
  result.file = readFile(filename);
  u32 *data = (u32 *)result.file.memory;
  result.width = data[0];
  result.height = data[1];
  result.pixels = (u8 *)&data[2];
  return result;
}

static LARGE_INTEGER prevCounter;
static u32 target = 1000 / 60;

void win32ClockTimer(void) {
  LARGE_INTEGER perfFrequency;
  QueryPerformanceFrequency(&perfFrequency); 
  LARGE_INTEGER newCounter;
  QueryPerformanceCounter(&newCounter);

  u64 counts = newCounter.QuadPart - prevCounter.QuadPart;
  u64 ms = (counts*1000) / perfFrequency.QuadPart;
  if (ms < target)
    Sleep((DWORD)(target - ms));
  else
    OutputDebugString("WARNING: Frame took too long!\n");
    
  QueryPerformanceCounter(&newCounter);
  prevCounter = newCounter;

  s8 buf[64];
  sprintf_s(buf, sizeof(buf), "Time taken: %llims\n", ms);
  OutputDebugString(buf);
}

#include "slingame.c"

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmd, int show) {
  initDebugLog();
  HWND window = win32CreateWindow(instance);
  if (!window)
    return -1;

  debugLog("Created window.");
  timeBeginPeriod(1);
  
  win32InitRawInput(window);
  debugLog("Initialised rawinput.");
  DX dx = win32InitD3D11(window);
  debugLog("Initialised DirectX11.");
  PLAYERINPUT pinput = {0};

  GAME game = {0};
  
  running = 1;
  init(&dx, &game);
  debugLog("Initialised game.");
  
  while (running) {
    MSG message;
    while (PeekMessageA(&message, window, 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      switch (message.message) {
      case WM_INPUT: {
        win32HandleRawInput(&message, &pinput);
      } break;
      default: {
        DispatchMessage(&message);
      } break;
      }
    }

    update(&dx, &game, &pinput);
    win32ClockTimer();
  }
  return 0;
}
