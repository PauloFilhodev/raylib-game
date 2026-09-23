#include "raylib.h"
#include "raymath.h"

// VARIAVEIS DO PLAYER
typedef struct Player
{
    Vector2 posicaoPlayer;
    float velocidade;
    float radius;
} Player;

void movimentarPlayer(Player *p)
{
    Vector2 direction = {0.0f, 0.0f};
    
    if (IsKeyDown(KEY_W))
    {
        direction.y -= 1.0f;
    }
    if (IsKeyDown(KEY_S))
    {
        direction.y += 1.0f;
    }
    if (IsKeyDown(KEY_D))
    {
        direction.x += 1.0f;
    }
    if (IsKeyDown(KEY_A))
    {
        direction.x -= 1.0f;
    }

    if (Vector2Length(direction) > 0.0f)
    {
        direction = Vector2Normalize(direction);

        p->posicaoPlayer.x += direction.x * p->velocidade * GetFrameTime();
        p->posicaoPlayer.y += direction.y * p->velocidade * GetFrameTime();
    }
}

int main()
{
    InitWindow(1280, 720, "Byte Delivery");
    SetTargetFPS(60);

    Player player = {
        .posicaoPlayer = {640, 360},
        .velocidade = 250.0f,
        .radius = 20.0f
    };
    Player *p_player = &player;

    while (!WindowShouldClose())
    {
        movimentarPlayer(p_player);
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawCircleV(player.posicaoPlayer, player.radius, RED);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}