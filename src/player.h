#ifndef PLAYER_H
#define PLAYER_H
#include <raylib.h>

#include "game_types.h"

void InitPlayer(Player *p, Vector2 position);
void UpdatePlayer(Player *p, Level *lvl, float dt);

// Aplica o efeito dos inimigos que estão encostando no jogador.
// Retorna true se o jogador encostou em um inimigo que mata (perdeu a fase).
bool CheckEnemyCollisions(Player *p, const Level *lvl);

void DrawPlayer(const Player *p);
#endif // PLAYER_H