#include "raylib.h"

int main ()
{
    InitWindow(800, 450, "Rayban");
    SetTargetFPS(60);
    
    Rectangle player = {250, 250, 50, 50};
    
    float *player_pos_x = &player.x;
    float *player_pos_y = &player.y;
    float player_speed = 5.0f;
    Rectangle player_passe = {*player_pos_x, *player_pos_y, 50, 50};

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        if (IsKeyDown(KEY_W)) {
            *player_pos_y -= player_speed;
        }
        if (IsKeyDown(KEY_S)) {
            *player_pos_y += player_speed;
        }
        if (IsKeyDown(KEY_D)) {
            *player_pos_x += player_speed;
        }
        if (IsKeyDown(KEY_A)) {
            *player_pos_x -= player_speed;
        }

        if (IsKeyDown(KEY_B))
        {
            DrawRectangleRec(player_passe, BLUE);
        }
        
        DrawRectangleRec(player, RED);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}