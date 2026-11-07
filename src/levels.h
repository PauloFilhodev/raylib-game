#ifndef LEVELS_H
#define LEVELS_H

#include "game_types.h"

// Lê um arquivo de fase. Retorna false (e loga o motivo) se o arquivo for inválido.
bool LoadLevel(const char *path, Level *lvl);

void UpdateEnemies(Level *lvl, float dt);

void DrawLevel(const Level *lvl);
void DrawEnemies(const Level *lvl);

int CountFilledSlots(const Level *lvl);

#endif