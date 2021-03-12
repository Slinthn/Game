#define MAX_ENTITIES 10

ENTITY *createEntity(GAME *game, u32 type) {
  ENTITY *result = 0;
  for (u32 i = 0; i < MAX_ENTITIES; i++) {
    if (game->entities[i].type == 0) {
      result = &game->entities[i];
      break;
    }
  }

  if (result != 0)
    result->type = type;
  
  return result;
}

void deleteEntity(GAME *game, ENTITY *entity) {
  entity->type = 0;
}
