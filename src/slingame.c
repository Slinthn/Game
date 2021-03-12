#include "math.c"
#include "entity.c"

void init(DX *dx, GAME *game) {
  game->shader = createShader(dx, "res\\defaultv.cso", "res\\defaultp.cso");
  dx->context->lpVtbl->VSSetShader(dx->context, game->shader.vShader, 0, 0);
  dx->context->lpVtbl->PSSetShader(dx->context, game->shader.pShader, 0, 0);
  
  game->samplerState = createSamplerState(dx);
  dx->context->lpVtbl->PSSetSamplers(dx->context, 0, 1, &game->samplerState);
  
  game->texture = loadBBMP("res\\bark.bbmp");
  game->textureView = createTextureView(dx, createTexture(dx, game->texture.width, game->texture.height, game->texture.pixels, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_SHADER_RESOURCE));
  dx->context->lpVtbl->PSSetShaderResources(dx->context, 0, 1, &game->textureView);
  freeFile(&game->texture.file);

  perspectiveMatrix4f(&game->cbuf0.perspective, 16 / (r32)9, 90, 0.01f, 100.0f);
  game->cbuffer[0] = createConstantBuffer(dx, &game->cbuf0, sizeof(DXCBUF0));
  game->cbuffer[1] = createConstantBuffer(dx, &game->cbuf1, sizeof(DXCBUF1)); // TODO(slin): Something wrong here
  dx->context->lpVtbl->VSSetConstantBuffers(dx->context, 0, 2, game->cbuffer);

  game->models[0] = loadBPLY("res\\bow.bply");
  game->models[1] = loadBPLY("res\\arrow.bply");
  game->dxmodels[0] = createModel(dx, &game->models[0]);
  game->dxmodels[1] = createModel(dx, &game->models[1]);
  freeFile(&game->models[0].file);
  freeFile(&game->models[1].file);

  ENTITY *entity = createEntity(game, 666);
  entity->position = (VECTOR3F){1, 0, 1};
  entity->rotation = (VECTOR3F){0, 90, 0};

  entity = createEntity(game, 666);
  entity->position = (VECTOR3F){0, 0, 0};
  entity->rotation = (VECTOR3F){0, 0, 0};
}

void handleInput(GAME *game, KEYBOARDINPUT *kinput, CONTROLLERINPUT *cinput) {
#define camSpeedMouse 0.5f
#define camSpeedController 5.5f
#define movSpeed 0.15f
  
  game->player.rotation.y += kinput->right.x*camSpeedMouse;
  game->player.rotation.x = clampf(game->player.rotation.x + kinput->right.y*camSpeedMouse, -90.0f, 90.0f);

  game->player.rotation.y += cinput->rStick.x*camSpeedController;
  game->player.rotation.x = clampf(game->player.rotation.x + cinput->rStick.y*camSpeedController, -90.0f, 90.0f);
  
  r32 rotCos = cosf(degToRad(game->player.rotation.y))*movSpeed;
  r32 rotSin = sinf(degToRad(game->player.rotation.y))*movSpeed;
  
  if (cinput->lStick.x != 0) {
    game->player.position.x += cinput->lStick.x*rotCos;
    game->player.position.z -= cinput->lStick.x*rotSin;
  }
  
  if (cinput->lStick.y != 0) {
    game->player.position.x -= cinput->lStick.y*rotSin;
    game->player.position.z -= cinput->lStick.y*rotCos;
  }
    
  if (kinput->left.x != 0) {
    game->player.position.x += kinput->left.x*rotCos;
    game->player.position.z -= kinput->left.x*rotSin;
  }
    
  if (kinput->left.y != 0) {
    game->player.position.x += kinput->left.y*rotSin;
    game->player.position.z += kinput->left.y*rotCos;
  }

  if (cinput->buttons & CONTROLLER_BUTTON_B) {
    CONTROLLEROUTPUT out = {0};
    out.lMotor = 255;

    outputController(out);
  }
}

void update(DX *dx, GAME *game, KEYBOARDINPUT *kinput, CONTROLLERINPUT *cinput) {
  handleInput(game, kinput, cinput);
  
  inverseTransformMatrix4f(&game->cbuf0.camera, game->player.position.x, game->player.position.y, game->player.position.z, game->player.rotation.x, game->player.rotation.y, game->player.rotation.z);

  dx->context->lpVtbl->ClearRenderTargetView(dx->context, dx->renderView, (r32[4]){0.5f, 1, 1, 1});
  dx->context->lpVtbl->ClearDepthStencilView(dx->context, dx->depthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1, 0);
  
  if (kinput->button.action0.down && kinput->button.action0.transitions == 0) {
    ENTITY *entity = createEntity(game, 666);
    entity->position = game->player.position;
    entity->rotation = game->player.rotation;
  }
  
  MATRIX4F identity;
  identityMatrix4f(&identity);

  MATRIX4F camera;
  inverseTransformMatrix4f(&camera, game->player.position.x, game->player.position.y, game->player.position.z, game->player.rotation.x, game->player.rotation.y, game->player.rotation.z);  
  
  for (u32 i = 0; i < MAX_ENTITIES; i++) {
    ENTITY *entity = &game->entities[i];
    if (entity->type == 0)
      continue;

    if (i == 0)
      CopyMemory(game->cbuf0.camera, identity, sizeof(MATRIX4F));
    else
      CopyMemory(game->cbuf0.camera, camera, sizeof(MATRIX4F));
    
    transformMatrix4f(&game->cbuf0.transform, entity->position.x, entity->position.y, entity->position.z, entity->rotation.x, entity->rotation.y, entity->rotation.z);

    updateConstantBuffer(dx, game->cbuffer[0], &game->cbuf0, sizeof(DXCBUF0));
    updateConstantBuffer(dx, game->cbuffer[1], &game->cbuf1, sizeof(DXCBUF1));
    
    dx->context->lpVtbl->IASetVertexBuffers(dx->context, 0, 1, &game->dxmodels[0].vertexBuffer, (u32[1]){8*sizeof(r32)}, (u32[1]){0});
    dx->context->lpVtbl->IASetIndexBuffer(dx->context, game->dxmodels[0].indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    dx->context->lpVtbl->DrawIndexed(dx->context, game->models[0].faceCount*3, 0, 0);
  }

  dx->swapchain->lpVtbl->Present(dx->swapchain, 1, 0); 
}
