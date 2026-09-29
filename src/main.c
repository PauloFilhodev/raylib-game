#include "raylib.h"
#include "raymath.h"
#include <stddef.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define MAP_ROWS 18
#define MAP_COLS 32
#define TILE_SIZE 40 // Tamanho de cada bloco na tela em pixels (20x32 = 640px / 15x32 = 480px)

#define MAX_MEM_BLOCKS 10
#define MAX_SLOT_BLOCKS 10

// VARIAVEIS DO BLOCO DE MEMORIA
typedef struct MemoryBlock
{
    bool isCarried;
    bool isPlaced;

    Rectangle hitbox;

    int gridX, gridY; // posição da grid (de acordo com os levels)
    Vector2 pixelPosition, spawnPosition;

    float radius;     // temporario
    Color blockColor; // temporario

    int blockValue;
} MemoryBlock;

typedef struct MemorySlot
{
    bool isFilled;

    Rectangle hitbox;
    int gridX, gridY;                     // posição da grid (de acordo com os levels)
    Vector2 pixelPosition, spawnPosition; // posição em pixeis no mapa

    Color slotColor; // temp

    int slotValue;
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
    int blockCount, blockSlots;
    const char *levelName;
    float timeLimit; // tempo limite

    int tileMap[MAP_ROWS][MAP_COLS]; // matriz de tiles do mapa

    MemoryBlock memoryBlock[MAX_MEM_BLOCKS];
    MemorySlot memorySlot[MAX_SLOT_BLOCKS];

    Vector2 spawnPlayer;
    Vector2 spawnMemoryBlock;
    Vector2 spawnMemorySlot;
} Level;

// VARIAVEIS DO PLAYER
typedef struct Player
{
    Vector2 playerPosition;
    float speed;
    float radius;

    // Referente ao que ele segura
    bool isHoldingBlock;
    MemoryBlock *carryingBlock;
} Player;

void LoadLevel(
    Level *lvl,
    char *map_name,
    int levelNumber,
    Player *p)
{
    lvl->levelNumber = levelNumber;
    lvl->levelName = map_name;

    for (int linha = 0; linha < MAP_ROWS; linha++)
    {
        for (int coluna = 0; coluna < MAP_COLS; coluna++)
        {
            int x = coluna * TILE_SIZE;
            int y = linha * TILE_SIZE;

            if (lvl->tileMap[linha][coluna] == TILE_SPAWN_PLAYER)
            {
                lvl->spawnPlayer = (Vector2){
                    x + TILE_SIZE / 2.0f,
                    y + TILE_SIZE / 2.0f};

                p->playerPosition = lvl->spawnPlayer;
            }
        }
    }

    // SLOTS
    for (int i = 0; i < lvl->blockSlots; i++)
    {
        MemorySlot *s = &lvl->memorySlot[i];

        int posX = s->gridX * TILE_SIZE;
        int posY = s->gridY * TILE_SIZE;

        s->pixelPosition = (Vector2){posX, posY};
    }
    // BLOCOS DE MEMÓRIA
    for (int i = 0; i < lvl->blockCount; i++)
    {
        MemoryBlock *m = &lvl->memoryBlock[i];

        int posX = m->gridX * TILE_SIZE + (TILE_SIZE / 2.0f);
        int posY = m->gridY * TILE_SIZE + (TILE_SIZE / 2.0f);

        m->pixelPosition = (Vector2){posX, posY};
    }
}
// ---------------------------------------------------------------------------------------------

// FUNÇÕES DE INICIO
void InitPlayer(Player *p, Vector2 position)
{
    p->playerPosition = position;
    p->radius = 15.0f;
    p->speed = 200.0f;
}

// ---------------------------------------------------------------------------------------------

// FUNÇÕES DE ATUALIZAÇÃO
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

void UpdateMemorySlot(Level *lvl, Player *p)
{
    for (int i = 0; i < lvl->blockSlots; i++)
    {
        MemorySlot *slot = &lvl->memorySlot[i];

        if (slot->isFilled &&
            Vector2Distance(p->carryingBlock->pixelPosition,
                            slot->pixelPosition) < 1.0f)
        {
            slot->isFilled = false;
            break;
        }
    }
}

void FitMemoryBlock(MemoryBlock *block, Level *lvl)
{
    for (int i = 0; i < lvl->blockSlots; i++)
    {
        MemorySlot *slot = &lvl->memorySlot[i];

        if (slot->isFilled)
            continue;

        Vector2 slotCenter = {
            slot->pixelPosition.x + TILE_SIZE / 2.0f,
            slot->pixelPosition.y + TILE_SIZE / 2.0f};

        float distance = Vector2Distance(
            block->pixelPosition,
            slotCenter);

        if (distance <= TILE_SIZE)
        {
            block->pixelPosition = slotCenter;
            block->isPlaced = true;
            slot->isFilled = true;

            return;
        }
    }
}

void PickUpOrReleaseBlock(Player *p, Level *lvl)
{
    if (IsKeyPressed(KEY_SPACE))
    {
        if (p->isHoldingBlock)
        {
            MemoryBlock *block = p->carryingBlock;

            block->pixelPosition = p->playerPosition;
            block->isCarried = false;

            FitMemoryBlock(block, lvl);

            p->carryingBlock = NULL;
            p->isHoldingBlock = false;
        }
        else
        {
            for (int i = 0; i < lvl->blockCount; i++)
            {
                float distance = Vector2Distance(p->playerPosition, lvl->memoryBlock[i].pixelPosition);
                
                if (distance <= p->radius + lvl->memoryBlock[i].radius)
                {
                    p->carryingBlock = &lvl->memoryBlock[i];
                    for (int i = 0; i < lvl->blockSlots; i++)
                    {
                        MemorySlot *slot = &lvl->memorySlot[i];

                        Vector2 slotCenter = {
                            slot->pixelPosition.x + TILE_SIZE / 2.0f,
                            slot->pixelPosition.y + TILE_SIZE / 2.0f};

                        if (slot->isFilled &&
                            Vector2Distance(p->carryingBlock->pixelPosition, slotCenter) < 1.0f)
                        {
                            slot->isFilled = false;
                            break;
                        }
                    }
                    p->carryingBlock->isCarried = true;
                    p->isHoldingBlock = true;
                    break;
                }
            }
        }
    }

    if (p->isHoldingBlock)
    {
        p->carryingBlock->pixelPosition.x = p->playerPosition.x;
        p->carryingBlock->pixelPosition.y = p->playerPosition.y - 15.0f;
    }
}

// ---------------------------------------------------------------------------------------------

// FUNÇÕES DRAW
void DrawPlayer(const Player *p)
{
    DrawCircle(p->playerPosition.x, p->playerPosition.y, p->radius, RED);
}

void DrawMemBlock(Level *lvl)
{
    for (int i = 0; i < lvl->blockCount; i++)
    {
        MemoryBlock *m = &lvl->memoryBlock[i];
        DrawCircle(m->pixelPosition.x, m->pixelPosition.y, m->radius, m->blockColor);
    }
}

void DrawMemSlot(Level *lvl)
{
    for (int i = 0; i < lvl->blockSlots; i++)
    {
        MemorySlot *s = &lvl->memorySlot[i];
        DrawText(TextFormat("%d", s->isFilled), s->pixelPosition.x, s->pixelPosition.y - 10.0f, 20, BLACK);
        DrawRectangle(s->pixelPosition.x, s->pixelPosition.y, TILE_SIZE, TILE_SIZE, s->slotColor);
    }
}
// ---------------------------------------------------------------------------------------------

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
                BLACK);
        }
    }
}

int main()
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Byte Delivery");
    SetTargetFPS(60);

    // DECLARAÇÃO DE VARIÁVEIS
    Player player = {0};
    Level level0 = {
        .levelNumber = 1,
        .blockCount = 3,
        .blockSlots = 3,
        .levelName = "Fase 1 - Tutorial",
        .timeLimit = 60.0f,
        .tileMap = {
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
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        .memoryBlock = {
            {.isPlaced = false, .isCarried = false, .gridX = 18, .gridY = 5, .blockValue = 1, .blockColor = PURPLE, .radius = 15.0f},
            {.isPlaced = false, .isCarried = false, .gridX = 18, .gridY = 5, .blockValue = 2, .blockColor = RED, .radius = 25.0f},
            {.isPlaced = false, .isCarried = false, .gridX = 18, .gridY = 5, .blockValue = 3, .blockColor = BLUE, .radius = 35.0f},
        },
        .memorySlot = {
            {.gridX = 16, .gridY = 1, .isFilled = false, .slotValue = 1, .slotColor = BLUE},
            {.gridX = 16, .gridY = 5, .isFilled = false, .slotValue = 2, .slotColor = YELLOW},
            {.gridX = 16, .gridY = 10, .isFilled = false, .slotValue = 3, .slotColor = GREEN},
            // {.gridX = 30, .gridY = 10, .isFilled = false, .slotValue = 2, .slotColor = YELLOW},
            // {.gridX = 35, .gridY = 10, .isFilled = false, .slotValue = 3, .slotColor = GREEN},
        }};

    // INICIALIZAÇÃO DE VARIÁVEIS
    InitPlayer(&player, (Vector2){0, 0});
    LoadLevel(&level0, "Fase 1", 1, &player);

    while (!WindowShouldClose())
    {
        // ATUALIZAÇÕES DO JOGO
        UpdatePlayer(&player);
        PickUpOrReleaseBlock(&player, &level0);

        // RENDERIZAÇÃO DO JOGO
        BeginDrawing();

        ClearBackground(RAYWHITE);
        // FUNÇÕES DRAW
        DrawLevel(&level0);
        DrawMemSlot(&level0);
        DrawMemBlock(&level0);
        DrawPlayer(&player);
        // INTERFACE
        DrawText(level0.levelName, 0, 0, 20, RED);
        if (player.isHoldingBlock)
        {
            DrawText("Block is being carried", 0, 20, 20, RED);
        }
        else
        {
            DrawText("Block is not being carried", 0, 20, 20, RED);
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}