#pragma warning(push, 0)
#include <windows.h>
#include <hidusage.h>
#include <d3d11.h>
#pragma warning(pop)

#ifdef SLINGAME_DEBUG
int sprintf_s(char *, size_t, char *, ...);
#endif

#include "types.h"

static b32 running;

FILE readFile(char *filename) {
  FILE result = {0};
  
  HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if (fileHandle == INVALID_HANDLE_VALUE)
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
  if (fileHandle == INVALID_HANDLE_VALUE)
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
  
  return CreateWindowA(wc.lpszClassName, "Slin Game", WS_OVERLAPPED|WS_MINIMIZEBOX|WS_CAPTION|WS_SYSMENU|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, 0, 0, instance, 0);
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

#define targetFPS (1000 / 60)

void win32ClockTimer(void) {
  static LARGE_INTEGER prevCounter;

  LARGE_INTEGER perfFrequency;
  QueryPerformanceFrequency(&perfFrequency); 
  LARGE_INTEGER newCounter;
  QueryPerformanceCounter(&newCounter);

  u64 counts = newCounter.QuadPart - prevCounter.QuadPart;
  u64 ms = (counts*1000) / perfFrequency.QuadPart;
  if (ms < targetFPS)
    Sleep((DWORD)(targetFPS - ms));
  else {
#ifdef SLINGAME_DEBUG
    OutputDebugString("WARNING: Frame took too long!\n");
#endif
  }
  
  QueryPerformanceCounter(&newCounter);
  prevCounter = newCounter;

#ifdef SLINGAME_DEBUG
  s8 buf[64];
  sprintf_s(buf, sizeof(buf), "Time taken: %llims\n", ms);
  OutputDebugString(buf);
#endif
}

#include "win32_rawinput.c"
#include "win32_dx11.c"
#include "slingame.c"

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmd, int show) {
  HWND window = win32CreateWindow(instance);
  if (!window)
    return -1;

  timeBeginPeriod(1);
  
  win32InitRawInput(window);
  DX dx = win32InitD3D11(window);
  KEYBOARDINPUT pinput = {0};
  CONTROLLERINPUT cinput = {0};
  
  GAME game = {0};
  
  running = 1;
  init(&dx, &game);
  
  while (running) {
    for (u32 i = 0; i < MAX_BUTTONS; i++)
      pinput.button.buttons[i].transitions++;
    
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

    cinput = inputController();
    update(&dx, &game, &pinput, &cinput);
    win32ClockTimer();
  }
  return 0;
}
