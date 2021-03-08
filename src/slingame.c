void init(DX *dx, GAME *game) {
  game->cbuffer = createConstantBuffer(dx, &game->cbuf, sizeof(DXCBUF0));

  game->shader = createShader(dx, "res\\defaultv.cso", "res\\defaultp.cso");
  debugLog("Loaded shaders.");
  dx->context->lpVtbl->VSSetShader(dx->context, game->shader.vShader, 0, 0);
  dx->context->lpVtbl->PSSetShader(dx->context, game->shader.pShader, 0, 0);
  debugLog("Set shaders.");
  
  game->samplerState = createSamplerState(dx);
  dx->context->lpVtbl->PSSetSamplers(dx->context, 0, 1, &game->samplerState);
  debugLog("Created and set samplers.");
  
  game->texture = loadBBMP("res\\bark.bbmp");
  game->textureView = createTextureView(dx, createTexture(dx, game->texture.width, game->texture.height, game->texture.pixels, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_SHADER_RESOURCE));
  dx->context->lpVtbl->PSSetShaderResources(dx->context, 0, 1, &game->textureView);
  debugLog("Creates and set textures.");

  perspectiveMatrix4f(&game->cbuf.perspective, 16 / (r32)9, 90, 0.01f, 100.0f);
  dx->context->lpVtbl->VSSetConstantBuffers(dx->context, 0, 1, &game->cbuffer);
  debugLog("Created and set constant buffers.");

  game->model = loadBPLY("res\\trunk.bply");
  createModel(dx, &game->model);
  debugLog("Created and loaded models.");    
}

void update(DX *dx, GAME *game, PLAYERINPUT *pinput) {
#define camSpeed 0.5f
#define movSpeed 0.15f
  
  game->player.rot[1] += pinput->r[0]*camSpeed;
  game->player.rot[0] = clampf(game->player.rot[0] + pinput->r[1]*camSpeed, -90.0f, 90.0f);

  r32 rotCos = cosf(degToRad(game->player.rot[1]))*movSpeed;
  r32 rotSin = sinf(degToRad(game->player.rot[1]))*movSpeed;
  if (pinput->l[0] != 0) {
    game->player.pos[0] += pinput->l[0]*rotCos;
    game->player.pos[2] -= pinput->l[0]*rotSin;
  }
    
  if (pinput->l[1] != 0) {
    game->player.pos[0] += pinput->l[1]*rotSin;
    game->player.pos[2] += pinput->l[1]*rotCos;
  }

  transformMatrix4f(&game->cbuf.transform, 0, 0, 0, 0, 0, 0);
  inverseCameraMatrix4f(&game->cbuf.camera, game->player.pos[0], game->player.pos[1], game->player.pos[2], game->player.rot[0], game->player.rot[1], game->player.rot[2]);
    
  updateConstantBuffer(dx, game->cbuffer, &game->cbuf, sizeof(DXCBUF0));
    
  dx->context->lpVtbl->ClearRenderTargetView(dx->context, dx->renderView, (r32[4]){1, 1, 1, 1});
  dx->context->lpVtbl->ClearDepthStencilView(dx->context, dx->depthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1, 0);
  dx->context->lpVtbl->DrawIndexed(dx->context, game->model.faceCount*3, 0, 0);
  dx->swapchain->lpVtbl->Present(dx->swapchain, 1, 0);

}
