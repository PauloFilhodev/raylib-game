#include "raylib.h"
#include "raymath.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define MAP_ROWS 18
#define MAP_COLS 32
#define TILE_SIZE 40 // Tamanho de cada bloco na tela em pixels (20x32 = 640px / 15x32 = 480px)

// VARIAVEIS DO PLAYER
typedef struct Player
{
    Vector2 playerPosition;
    float speed;
    float radius;
} Player;

// VARIAVEIS DO BLOCO DE MEMORIA
typedef struct MemoryBlock
{
    bool isCarried;
    bool isPlaced;

    Rectangle hitbox;

    int gridX, gridY; // posição da grid (de acordo com os levels)
    Vector2 pixelPosition;

    float radius;

    const char *blockValue;
} MemoryBlock;

typedef struct MemorySlot
{
    bool isFilled;

    Rectangle hitbox;
    int gridX, gridY;      // posição da grid (de acordo com os levels)
    Vector2 pixelPosition; // posição em pixeis no mapa

    const char *slotValue;
} MemorySlot;

typedef enum
{
    TILE_FLOOR = 0,
    TILE_WALL = 1,
    TILE_SPAWN_PLAYER = 2,
    TILE_SPAWN_BLOCK = 3,
    TILE_SLOT_RAM = 4
} TileType;

// CONSTRUÇÃO DO LEVEL DO MAPA
typedef struct Level
{
    int levelNumber;
    const char *levelName;
    float timeLimit; // tempo limite

    int tileMap[MAP_ROWS][MAP_COLS]; // matriz de tiles do mapa

    MemoryBlock memoryBlock;
    MemorySlot memorySlot;

    Vector2 spawnPlayer;
    Vector2 spawnMemoryBlock;
    Vector2 spawnMemorySlot;
} Level;

void LoadLevel(
    Level *lvl,
    int map[MAP_ROWS][MAP_COLS],
    char *map_name,
    int levelNumber,
    Player *p,
    MemoryBlock *m
)
{
    lvl->levelNumber = levelNumber;
    lvl->levelName = map_name;

    for (int linha = 0; linha < MAP_ROWS; linha++)
    {
        for (int coluna = 0; coluna < MAP_COLS; coluna++)
        {
            int x = coluna * TILE_SIZE;
            int y = linha * TILE_SIZE;

            lvl->tileMap[linha][coluna] = map[linha][coluna];

            if (lvl->tileMap[linha][coluna] == TILE_SPAWN_PLAYER)
            {
                lvl->spawnPlayer = (Vector2){
                    x + TILE_SIZE / 2.0f,
                    y + TILE_SIZE / 2.0f};

                p->playerPosition = lvl->spawnPlayer;
            }
            if (lvl->tileMap[linha][coluna] == TILE_SPAWN_BLOCK)
            {
                lvl->spawnMemoryBlock = (Vector2){
                    x + TILE_SIZE / 2,
                    y + TILE_SIZE / 2};
                m->pixelPosition = lvl->spawnMemoryBlock;
            }
        }
    }
}

void DrawLevel(Level *lvl)
{
    for (int linha = 0; linha < MAP_ROWS; linha++)
    {
        for (int coluna = 0; coluna < MAP_COLS; coluna++)
        {
            // COLUNAS E LINHAS CORRESPONDENTES NO MAPA
            int x = coluna * TILE_SIZE;
            int y = linha * TILE_SIZE;

            switch (lvl->tileMap[linha][coluna])
            {
            case TILE_FLOOR:
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, (Color){46, 70, 89, 255});
                break;
            case TILE_WALL:
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, BLACK);
                break;
            case TILE_SPAWN_PLAYER:
                DrawRectangle(
                    x, y,
                    TILE_SIZE, TILE_SIZE,
                    GRAY);
                break;
            case TILE_SPAWN_BLOCK:
                DrawRectangle(
                    x, y,
                    TILE_SIZE, TILE_SIZE,
                    GREEN);
                break;
            case TILE_SLOT_RAM:
                DrawRectangle(
                    x, y,
                    TILE_SIZE, TILE_SIZE,
                    PURPLE);
                break;
            default:
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, GOLD);
                break;
            }

            DrawRectangleLines(
                x, y,
                TILE_SIZE, TILE_SIZE,
                BLACK
            );
        }
    }
}

// FUNÇÕES DO PLAYER
void InitPlayer(Player *p, Vector2 posicao)
{
    p->playerPosition = posicao;
    p->radius = 15.0f;
    p->speed = 200.0f;
}

void UpdatePlayer(Player *p)
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

        p->playerPosition.x += direction.x * p->speed * GetFrameTime();
        p->playerPosition.y += direction.y * p->speed * GetFrameTime();
    }

}

void ManageBlock(Player *p, MemoryBlock *block)
{
    if (IsKeyDown(KEY_SPACE))
    {
        if (block->isCarried)
        {
            float distance = Vector2Distance(p->playerPosition, block->pixelPosition);

            if (distance <= p->radius + block->radius)
            {
                block->isCarried = true;
                block->pixelPosition.x = p->playerPosition.x;
                block->pixelPosition.y = p->playerPosition.y + 50;
            }
        }
        else
        {
            if (block->isCarried)
            {
                block->isCarried = false;

                block->pixelPosition.x = p->playerPosition.x;
                block->pixelPosition.y = p->playerPosition.y;

                return;
            }
        }
    }
}

void DrawPlayer(const Player *p)
{
    DrawCircle(p->playerPosition.x, p->playerPosition.y, p->radius, RED);
}

void DrawMemBlock(const MemoryBlock *m)
{
    DrawCircle(m->pixelPosition.x, m->pixelPosition.y, m->radius, (Color){161, 21, 168, 255});
}

int main()
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Byte Delivery");
    SetTargetFPS(60);

    // DECLARAÇÃO DE VARIÁVEIS
    Player player = {0};
    Level level = {0};
    MemoryBlock mem_block = {0};
    MemorySlot mem_slot = {0};

    // MAPA
    int mapa1[MAP_ROWS][MAP_COLS] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 3, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};
    // INICIALIZAÇÃO DE VARIÁVEIS
    InitPlayer(&player, (Vector2){0, 0});
    LoadLevel(&level, mapa1, "Fase 1", 1, &player, &mem_block);

    while (!WindowShouldClose())
    {
        // ATUALIZAÇÕES DO JOGO
        UpdatePlayer(&player);
        ManageBlock(&player, &mem_block);

        // RENDERIZAÇÃO DO JOGO
        BeginDrawing();

        ClearBackground(RAYWHITE);
        // FUNÇÕES DRAW
        DrawLevel(&level);
        DrawPlayer(&player);
        DrawMemBlock(&mem_block);
        // INTERFACE
        DrawText(level.levelName, 0, 0, 20, RED);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}