#include <raylib.h>
#include <raymath.h>

#include <stddef.h>

#include "player.h"
#include "collision.h"
#include "assets.h"

// Duração dos efeitos (segundos)
#define STUN_TIME 1.5f
#define STUN_IMMUNITY 2.5f // tempo sem sofrer stun/reverse de novo após ser atordoado
#define REVERSE_TIME 4.0f
#define REVERSE_IMMUNITY 2.0f

// Tamanho do sprite em pixels (a imagem original tem 40x40)
#define PLAYER_SPRITE_SIZE 56.0f

void InitPlayer(Player *p, Vector2 position)
{
    p->playerPosition = position;
    p->radius = 15.0f;
    p->speed = 200.0f;
    p->isHoldingBlock = false;
    p->carryingBlock = NULL;
    p->stunTimer = 0.0f;
    p->reverseTimer = 0.0f;
    p->immuneTimer = 0.0f;
}

void UpdatePlayer(Player *p, Level *lvl, float dt)
{
    if (p->stunTimer > 0.0f)
        p->stunTimer -= dt;
    if (p->reverseTimer > 0.0f)
        p->reverseTimer -= dt;
    if (p->immuneTimer > 0.0f)
        p->immuneTimer -= dt;

    // Atordoado: não se move
    if (p->stunTimer <= 0.0f)
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

        // Controles invertidos
        if (p->reverseTimer > 0.0f)
        {
            direction.x = -direction.x;
            direction.y = -direction.y;
        }

        if (Vector2Length(direction) > 0.0f)
        {
            direction = Vector2Normalize(direction);

            p->playerPosition.x += direction.x * p->speed * dt;
            p->playerPosition.y += direction.y * p->speed * dt;
        }
    }

    // Sempre resolve, mesmo parado (ex.: spawn encostado em parede)
    ResolveWallCollisions(p, lvl);
}

bool CheckEnemyCollisions(Player *p, const Level *lvl)
{
    for (int i = 0; i < lvl->enemyCount; i++)
    {
        const Enemy *e = &lvl->enemy[i];

        Rectangle box = {e->pos.x - e->half, e->pos.y - e->half, e->half * 2.0f, e->half * 2.0f};

        if (!CheckCollisionCircleRec(p->playerPosition, p->radius, box))
            continue;

        switch (e->type)
        {
        case ENEMY_KILL:
            return true; // mata mesmo durante a imunidade

        case ENEMY_STUN:
            if (p->immuneTimer <= 0.0f)
            {
                p->stunTimer = STUN_TIME;
                p->immuneTimer = STUN_IMMUNITY;
            }
            break;

        case ENEMY_REVERSE:
            if (p->immuneTimer <= 0.0f)
            {
                p->reverseTimer = REVERSE_TIME;
                p->immuneTimer = REVERSE_IMMUNITY;
            }
            break;
        }
    }

    return false;
}

void DrawPlayer(const Player *p)
{
    Color color = WHITE;

    if (p->stunTimer > 0.0f)
        color = YELLOW;
    else if (p->reverseTimer > 0.0f)
        color = MAGENTA;

    // Pisca (fica mais transparente) enquanto esta imune
    if (p->immuneTimer > 0.0f && ((int)(p->immuneTimer * 10.0f) % 2 == 0))
        color = Fade(color, 0.35f);

    Texture2D t = gAssets.player;

    if (t.id == 0)
    {
        // Sem textura: circulo como antes
        DrawCircle((int)p->playerPosition.x, (int)p->playerPosition.y, p->radius, color);
        DrawCircleLines((int)p->playerPosition.x, (int)p->playerPosition.y, p->radius, BLACK);
        return;
    }

    float size = PLAYER_SPRITE_SIZE; // so o desenho; a hitbox continua com p->radius
    Rectangle src = {0, 0, (float)t.width, (float)t.height};
    Rectangle dst = {p->playerPosition.x, p->playerPosition.y, size, size};
    Vector2 origin = {size / 2.0f, size / 2.0f};

    DrawTexturePro(t, src, dst, origin, 0.0f, color);
}
