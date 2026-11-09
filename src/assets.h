#ifndef ASSETS_H
#define ASSETS_H

#include "raylib.h"

// Todas as texturas do jogo ficam aqui. Carrega uma vez, descarrega uma vez.
typedef struct Assets
{
    Texture2D player;
    Texture2D memBlock;
    Texture2D enemy[3]; // indexado por EnemyType
} Assets;

extern Assets gAssets;

void LoadAssets(void);   // chamar DEPOIS do InitWindow
void UnloadAssets(void); // chamar ANTES do CloseWindow

#endif
