#ifndef PLAYER_H
#define PLAYER_H
#include <raylib.h>

#include "game_types.h"

void InitPlayer(Player* p, Vector2 position);
void UpdatePlayer(Player* p, Level* lvl);
void DrawPlayer(const Player* p);
#endif // PLAYER_H