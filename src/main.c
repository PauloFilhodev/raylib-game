#include "raylib.h"

int main ()
{
    InitWindow(800, 450, "Meu primeiro jogo");

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawText(
            "Raylib funcionando!",
            250,
            200,
            30,
            BLACK
        );

        EndDrawing();
    }

    CloseWindow();

    return 0;
}