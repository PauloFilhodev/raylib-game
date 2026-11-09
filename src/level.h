#ifndef LEVEL_H
#define LEVEL_H

#include "game_types.h"

bool LoadLevel(const char *path, Level *lvl);
void DrawLevel(const Level *lvl);
bool IsWall(const Level *lvl, int col, int row);

#endif
