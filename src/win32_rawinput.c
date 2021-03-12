static HANDLE win32ControllerFile;

#pragma pack(push, 1)
typedef struct {
  u8 reportId;
  u8 lStickX;
  u8 lStickY;
  u8 rStickX;
  u8 rStickY;
  u8 buttons[3];
  u32 randomShit[100];
  u8 lTrigger;
  u8 rTrigger;
  u16 timestamp;
  u8 battery;
  u16 gyroX;
  u16 gyroY;
  u16 gyroZ;
  u16 accelX;
  u16 accelY;
  u16 accelZ;
} DS4INPUT;
#pragma pack(pop)

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

  RegisterRawInputDevices(ri, sizeofarray(ri), sizeof(RAWINPUTDEVICE));

  RAWINPUTDEVICELIST devices[20];
  u32 deviceCount = sizeofarray(devices);
  
  GetRawInputDeviceList(devices, &deviceCount, sizeof(RAWINPUTDEVICELIST));
  GetRawInputDeviceList(0, &deviceCount, sizeof(RAWINPUTDEVICELIST));

  RAWINPUTDEVICELIST *selected = 0;
  for (u32 i = 0; i < deviceCount; i++) {
    if (devices[i].dwType == RIM_TYPEHID) {
      RID_DEVICE_INFO deviceInfo;
      u32 size = sizeof(RID_DEVICE_INFO);
      GetRawInputDeviceInfoA(devices[i].hDevice, RIDI_DEVICEINFO, &deviceInfo, &size);

      if (deviceInfo.hid.usUsage != HID_USAGE_GENERIC_GAMEPAD)
        continue;
            
      selected = &devices[i];
      break;
    }
  }

  if (selected != 0) {
    s8 filename[128];
    u32 size = sizeof(filename);
    GetRawInputDeviceInfoA(selected->hDevice, RIDI_DEVICENAME, filename, &size);
    
    win32ControllerFile = CreateFileA(filename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    assert(win32ControllerFile != INVALID_HANDLE_VALUE);
  }  
}

void win32HandleRawInput(MSG *message, KEYBOARDINPUT *input) {
  static b32 mouseCapture = 0;
  
  u32 size = sizeof(RAWINPUT);
  RAWINPUT ri;
  GetRawInputData((HRAWINPUT)message->lParam, RID_INPUT, &ri, &size, sizeof(RAWINPUTHEADER));
  if (mouseCapture && ri.header.dwType == RIM_TYPEKEYBOARD) {
    u16 key = ri.data.keyboard.VKey;
    b32 down = !(ri.data.keyboard.Flags & RI_KEY_BREAK);
    switch (key) {
    case 'W': {
      input->left.y = (r32)down;
    } break;
    case 'A': {
      input->left.x = (r32)-down;
    } break;
    case 'S': {
      input->left.y = (r32)-down;
    } break;
    case 'D': {
      input->left.x = (r32)down;
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
      input->right.x = (r32)ri.data.mouse.lLastX;
      input->right.y = (r32)ri.data.mouse.lLastY;
      if (ri.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
        input->button.action0.down = 1;
        input->button.action0.transitions = 0;
      } else if (ri.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
        input->button.action0.down = 0;
        input->button.action0.transitions = 0;
      }
    }
  }
}

r32 win32ConstrainJoystick(u8 in) {
  r32 result;
  if (in <= 0)
    result = (in - 128) / 128.0f;
  else
    result = (in - 127) / 128.0f;

  if ((result < 0 && result > -CONTROLLER_DEADZONE) || (result > 0 && result < CONTROLLER_DEADZONE))
    return 0;
  else
    return result;
}

CONTROLLERINPUT inputController(void) {
  CONTROLLERINPUT result = {0};
  DS4INPUT ds4 = {0};
  DWORD read;
  ReadFile(win32ControllerFile, &ds4, sizeof(DS4INPUT), &read, 0);

  if (ds4.reportId != 0x1)
    return result;
  
  result.lStick.x = win32ConstrainJoystick(ds4.lStickX);
  result.lStick.y = win32ConstrainJoystick(ds4.lStickY);
  result.rStick.x = win32ConstrainJoystick(ds4.rStickX);
  result.rStick.y = win32ConstrainJoystick(ds4.rStickY);

  if (ds4.buttons[0] & 0x80)
    result.buttons |= CONTROLLER_BUTTON_Y;

  if (ds4.buttons[0] & 0x40)
    result.buttons |= CONTROLLER_BUTTON_B;

  if (ds4.buttons[0] & 0x20)
    result.buttons |= CONTROLLER_BUTTON_A;

  if (ds4.buttons[0] & 0x10)
    result.buttons |= CONTROLLER_BUTTON_X;

  result.battery = ds4.battery;
  result.gyro.x = ds4.gyroX;
  result.gyro.y = ds4.gyroY;
  result.gyro.z = ds4.gyroZ;
  result.accel.x = ds4.accelX;
  result.accel.y = ds4.accelY;
  result.accel.z = ds4.accelZ;
  
  return result;
}

void outputController(CONTROLLEROUTPUT out) {
  if (win32ControllerFile == INVALID_HANDLE_VALUE)
    return;
  
  u8 buf[32] = {0};
  buf[0] = 5;
  buf[1] = 0xFF;
  buf[4] = out.rMotor;
  buf[5] = out.lMotor;
  buf[6] = out.r;
  buf[7] = out.g;
  buf[8] = out.b;
  DWORD written;
  WriteFile(win32ControllerFile, buf, sizeof(buf), &written, 0);
}
