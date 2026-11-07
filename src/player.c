#include <raylib.h>
#include <raymath.h>

#include "player.h"
#include "game_types.h"
#include "collision.c"


void InitPlayer(Player *p, Vector2 position)
{
    p->playerPosition = position;
    p->radius = 15.0f;
    p->speed = 200.0f;
    p->isHoldingBlock = false;
    p->carryingBlock = NULL;
}

void UpdatePlayer(Player *p, Level *lvl)
{
    Vector2 direction = {0.0f, 0.0f};

    if (IsKeyDown(KEY_W))
        direction.y -= 1.0f;
    if (IsKeyDown(KEY_S))
        direction.y += 1.0f;
    if (IsKeyDown(KEY_D))
        direction.x += 1.0f;
    if (IsKeyDown(KEY_A))
        direction.x -= 1.0f;

    if (Vector2Length(direction) > 0.0f)
    {
        direction = Vector2Normalize(direction);

        float dt = GetFrameTime();
        p->playerPosition.x += direction.x * p->speed * dt;
        p->playerPosition.y += direction.y * p->speed * dt;
    }

    // Sempre resolve, mesmo parado (ex.: spawn encostado em parede)
    ResolveWallCollisions(p, lvl);
}

void DrawPlayer(const Player *p)
{
    DrawCircle((int)p->playerPosition.x, (int)p->playerPosition.y, p->radius, RED);
}

