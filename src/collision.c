#include "raylib.h"
#include "raymath.h"

#include <math.h>

#include "collision.h"

// Fora do mapa conta como parede
bool IsWallTile(const Level *lvl, int row, int col)
{
    if (row < 0 || row >= MAP_ROWS || col < 0 || col >= MAP_COLS)
        return true;

    return lvl->tileMap[row][col] == TILE_WALL;
}

// COLISÃO COM PAREDES (resolução por empurrão)
//
// Em vez de rejeitar o movimento quando há colisão, o jogador se move livremente
// e depois é empurrado para fora das paredes ao longo da normal do contato
// (ponto mais próximo do retângulo ao centro do círculo). Isso dá deslizamento
// suave nas paredes e nas quinas, sem "grudar".
void ResolveWallCollisions(Player *p, const Level *lvl)
{
    // Algumas passadas: empurrar para fora de uma parede pode jogar contra outra
    for (int pass = 0; pass < 4; pass++)
    {
        Vector2 pos = p->playerPosition;

        int colStart = (int)floorf((pos.x - p->radius) / TILE_SIZE);
        int colEnd = (int)floorf((pos.x + p->radius) / TILE_SIZE);
        int rowStart = (int)floorf((pos.y - p->radius) / TILE_SIZE);
        int rowEnd = (int)floorf((pos.y + p->radius) / TILE_SIZE);

        bool found = false;
        float bestDist = p->radius;
        Vector2 bestPush = {0.0f, 0.0f};

        for (int row = rowStart; row <= rowEnd; row++)
        {
            for (int col = colStart; col <= colEnd; col++)
            {
                if (!IsWallTile(lvl, row, col))
                    continue;

                Rectangle wall = {col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE};

                Vector2 closest = {
                    Clamp(pos.x, wall.x, wall.x + wall.width),
                    Clamp(pos.y, wall.y, wall.y + wall.height)};

                Vector2 diff = Vector2Subtract(pos, closest);
                float dist = Vector2Length(diff);

                if (dist >= p->radius)
                    continue;

                Vector2 push;

                if (dist > 0.0001f)
                {
                    // Empurra ao longo da normal até o círculo só encostar
                    push = Vector2Scale(diff, (p->radius - dist) / dist);
                }
                else
                {
                    // Centro dentro do tile: sai pela face mais próxima
                    float dl = pos.x - wall.x;
                    float dr = wall.x + wall.width - pos.x;
                    float dt = pos.y - wall.y;
                    float db = wall.y + wall.height - pos.y;
                    float m = fminf(fminf(dl, dr), fminf(dt, db));

                    if (m == dl)
                        push = (Vector2){-(dl + p->radius), 0.0f};
                    else if (m == dr)
                        push = (Vector2){dr + p->radius, 0.0f};
                    else if (m == dt)
                        push = (Vector2){0.0f, -(dt + p->radius)};
                    else
                        push = (Vector2){0.0f, db + p->radius};
                }

                // Resolve só a penetração mais profunda por passada.
                // Evita "colisões fantasma" na junção entre dois tiles alinhados.
                if (!found || dist < bestDist)
                {
                    found = true;
                    bestDist = dist;
                    bestPush = push;
                }
            }
        }

        if (!found)
            break;

        p->playerPosition = Vector2Add(p->playerPosition, bestPush);
    }
}