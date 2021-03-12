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
  } else {
    D3D11_SUBRESOURCE_DATA sd = {0};
    sd.pSysMem = memory;
    sd.SysMemPitch = width*sizeof(u8)*4;
    
    dx->device->lpVtbl->CreateTexture2D(dx->device, &desc, &sd, &texture);
    return texture;
  }
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

DX win32InitD3D11(HWND window) {
  DX result = {0};

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

#ifdef SLINGAME_DEBUG
  u32 flags = D3D11_CREATE_DEVICE_SINGLETHREADED|D3D11_CREATE_DEVICE_DEBUG;
#else
  u32 flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#endif
  
  D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, flags, (D3D_FEATURE_LEVEL[1]){D3D_FEATURE_LEVEL_11_0}, 1, D3D11_SDK_VERSION, &scd, &result.swapchain, &result.device, 0, &result.context);

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
  
  // NOTE(slin): render buffers
  IID id = {3700319219, 53547, 18770, {180, 123, 94, 69, 2, 106, 134, 45}};

  ID3D11Resource *backbuffer;
  result.swapchain->lpVtbl->GetBuffer(result.swapchain, 0, &id, &backbuffer);
  result.device->lpVtbl->CreateRenderTargetView(result.device, backbuffer, 0, &result.renderView);
  backbuffer->lpVtbl->Release(backbuffer);

  ID3D11Texture2D *depthTexture = createTexture(&result, width, height, 0, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL);
  result.device->lpVtbl->CreateDepthStencilView(result.device, (ID3D11Resource *)depthTexture, 0, &result.depthStencilView);

  result.context->lpVtbl->OMSetRenderTargets(result.context, 1, &result.renderView, result.depthStencilView);
  return result;
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

DXMODEL createModel(DX *dx, MODEL *model) {
  DXMODEL result = {0};

  D3D11_BUFFER_DESC ibd = {0};
  ibd.ByteWidth = model->faceCount*3*sizeof(u32);
  ibd.Usage = D3D11_USAGE_IMMUTABLE;
  ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
  ibd.CPUAccessFlags = 0;
  ibd.MiscFlags = 0;
  ibd.StructureByteStride = sizeof(u32);

  D3D11_SUBRESOURCE_DATA isd = {0};
  isd.pSysMem = model->faces;

  dx->device->lpVtbl->CreateBuffer(dx->device, &ibd, &isd, &result.indexBuffer);

  D3D11_BUFFER_DESC vbd = {0};
  vbd.ByteWidth = model->vertexCount*8*sizeof(r32);
  vbd.Usage = D3D11_USAGE_IMMUTABLE;
  vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  vbd.CPUAccessFlags = 0;
  vbd.MiscFlags = 0;
  vbd.StructureByteStride = 8*sizeof(r32);
    
  D3D11_SUBRESOURCE_DATA vsd = {0};
  vsd.pSysMem = model->vertices;

  dx->device->lpVtbl->CreateBuffer(dx->device, &vbd, &vsd, &result.vertexBuffer);
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
