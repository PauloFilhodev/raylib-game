#ifndef COLLISION_H
#define COLLISION_H

#include "raylib.h"
#include "game_types.h"

void ResolveWallCollisions(Player *p, const Level *lvl);
bool IsWallTile(const Level *lvl, int row, int col);

#endif