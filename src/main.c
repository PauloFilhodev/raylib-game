#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define MAP_ROWS 18
#define MAP_COLS 32
#define TILE_SIZE 40 // Tamanho de cada bloco na tela em pixels

#define MAX_MEM_BLOCKS 10
#define MAX_SLOT_BLOCKS 10

// VARIAVEIS DO BLOCO DE MEMORIA
typedef struct MemoryBlock
{
    bool isCarried;
    bool isPlaced;

    Rectangle hitbox; // ainda não usado

    int gridX, gridY; // posição da grid (de acordo com os levels)
    Vector2 pixelPosition, spawnPosition;

    float radius;     // temporario
    Color blockColor; // temporario

    int blockValue;
} MemoryBlock;

typedef struct MemorySlot
{
    bool isFilled;

    Rectangle hitbox; // retângulo do tile do slot (preenchido no LoadLevel)
    int gridX, gridY; // posição da grid (de acordo com os levels)
    Vector2 pixelPosition, spawnPosition; // canto superior esquerdo do slot, em pixels

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

// ---------------------------------------------------------------------------------------------
// AUXILIARES

static Vector2 SlotCenter(const MemorySlot *slot)
{
    return (Vector2){
        slot->pixelPosition.x + TILE_SIZE / 2.0f,
        slot->pixelPosition.y + TILE_SIZE / 2.0f};
}

// Fora do mapa conta como parede
static bool IsWallTile(const Level *lvl, int row, int col)
{
    if (row < 0 || row >= MAP_ROWS || col < 0 || col >= MAP_COLS)
        return true;

    return lvl->tileMap[row][col] == TILE_WALL;
}

// ---------------------------------------------------------------------------------------------
// CARREGAMENTO DO LEVEL

void LoadLevel(Level *lvl, const char *map_name, int levelNumber, Player *p)
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

        s->pixelPosition = (Vector2){s->gridX * TILE_SIZE, s->gridY * TILE_SIZE};
        s->spawnPosition = s->pixelPosition;
        s->hitbox = (Rectangle){s->pixelPosition.x, s->pixelPosition.y, TILE_SIZE, TILE_SIZE};
    }

    // BLOCOS DE MEMÓRIA
    for (int i = 0; i < lvl->blockCount; i++)
    {
        MemoryBlock *m = &lvl->memoryBlock[i];

        m->pixelPosition = (Vector2){
            m->gridX * TILE_SIZE + (TILE_SIZE / 2.0f),
            m->gridY * TILE_SIZE + (TILE_SIZE / 2.0f)};
        m->spawnPosition = m->pixelPosition;
    }
}

// ---------------------------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------------------------
// FUNÇÕES DE INICIO
void InitPlayer(Player *p, Vector2 position)
{
    p->playerPosition = position;
    p->radius = 15.0f;
    p->speed = 200.0f;
    p->isHoldingBlock = false;
    p->carryingBlock = NULL;
}

// ---------------------------------------------------------------------------------------------
// FUNÇÕES DE ATUALIZAÇÃO
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

// Libera o slot ocupado por um bloco (se ele estiver encaixado em algum)
static void ReleaseSlotOfBlock(MemoryBlock *block, Level *lvl)
{
    if (!block->isPlaced)
        return;

    for (int i = 0; i < lvl->blockSlots; i++)
    {
        MemorySlot *slot = &lvl->memorySlot[i];

        if (slot->isFilled &&
            Vector2Distance(block->pixelPosition, SlotCenter(slot)) < 1.0f)
        {
            slot->isFilled = false;
            break;
        }
    }

    block->isPlaced = false;
}

// Tenta encaixar o bloco em um slot livre próximo E com o mesmo valor
void FitMemoryBlock(MemoryBlock *block, Level *lvl)
{
    for (int i = 0; i < lvl->blockSlots; i++)
    {
        MemorySlot *slot = &lvl->memorySlot[i];

        if (slot->isFilled)
            continue;

        if (slot->slotValue != block->blockValue)
            continue;

        Vector2 slotCenter = SlotCenter(slot);

        if (Vector2Distance(block->pixelPosition, slotCenter) <= TILE_SIZE)
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
            // SOLTAR
            MemoryBlock *block = p->carryingBlock;

            block->pixelPosition = p->playerPosition;
            block->isCarried = false;

            FitMemoryBlock(block, lvl);

            p->carryingBlock = NULL;
            p->isHoldingBlock = false;
        }
        else
        {
            // PEGAR: escolhe o bloco mais próximo ao alcance
            MemoryBlock *nearest = NULL;
            float nearestDist = 0.0f;

            for (int i = 0; i < lvl->blockCount; i++)
            {
                MemoryBlock *b = &lvl->memoryBlock[i];

                if (b->isCarried)
                    continue;

                float distance = Vector2Distance(p->playerPosition, b->pixelPosition);

                if (distance <= p->radius + b->radius &&
                    (nearest == NULL || distance < nearestDist))
                {
                    nearest = b;
                    nearestDist = distance;
                }
            }

            if (nearest != NULL)
            {
                ReleaseSlotOfBlock(nearest, lvl);

                nearest->isCarried = true;
                p->carryingBlock = nearest;
                p->isHoldingBlock = true;
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
    DrawCircle((int)p->playerPosition.x, (int)p->playerPosition.y, p->radius, RED);
}

void DrawMemBlock(const Level *lvl)
{
    for (int i = 0; i < lvl->blockCount; i++)
    {
        const MemoryBlock *m = &lvl->memoryBlock[i];
        DrawCircle((int)m->pixelPosition.x, (int)m->pixelPosition.y, m->radius, m->blockColor);
        DrawCircleLines((int)m->pixelPosition.x, (int)m->pixelPosition.y, m->radius, BLACK);
    }
}

void DrawMemSlot(const Level *lvl)
{
    for (int i = 0; i < lvl->blockSlots; i++)
    {
        const MemorySlot *s = &lvl->memorySlot[i];
        int x = (int)s->pixelPosition.x;
        int y = (int)s->pixelPosition.y;

        // Retângulo primeiro, texto depois (antes o texto ficava escondido)
        DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, Fade(s->slotColor, 0.6f));
        DrawRectangleLines(x, y, TILE_SIZE, TILE_SIZE, WHITE);
        DrawText(TextFormat("%d", s->slotValue), x + 4, y + 2, 16, WHITE);
        DrawText(TextFormat("%d", s->isFilled), x + 4, y - 18, 16, BLACK);
    }
}

void DrawLevel(const Level *lvl)
{
    for (int linha = 0; linha < MAP_ROWS; linha++)
    {
        for (int coluna = 0; coluna < MAP_COLS; coluna++)
        {
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
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, GRAY);
                break;
            case TILE_SPAWN_BLOCK:
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, GREEN);
                break;
            case TILE_SLOT_RAM:
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, PURPLE);
                break;
            default:
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, GOLD);
                break;
            }

            DrawRectangleLines(x, y, TILE_SIZE, TILE_SIZE, BLACK);
        }
    }
}

// ---------------------------------------------------------------------------------------------

int main(void)
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
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 4, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
        // Blocos espalhados (antes os 3 nasciam empilhados no mesmo tile)
        .memoryBlock = {
            {.isPlaced = false, .isCarried = false, .gridX = 18, .gridY = 5, .blockValue = 1, .blockColor = PURPLE, .radius = 15.0f},
            {.isPlaced = false, .isCarried = false, .gridX = 21, .gridY = 5, .blockValue = 2, .blockColor = RED, .radius = 25.0f},
            {.isPlaced = false, .isCarried = false, .gridX = 25, .gridY = 5, .blockValue = 3, .blockColor = BLUE, .radius = 35.0f},
        },
        // Cor de cada slot = cor do bloco com o mesmo valor
        .memorySlot = {
            {.gridX = 16, .gridY = 1, .isFilled = false, .slotValue = 1, .slotColor = PURPLE},
            {.gridX = 16, .gridY = 5, .isFilled = false, .slotValue = 2, .slotColor = RED},
            {.gridX = 16, .gridY = 10, .isFilled = false, .slotValue = 3, .slotColor = BLUE},
        }};

    // INICIALIZAÇÃO DE VARIÁVEIS
    InitPlayer(&player, (Vector2){0, 0});
    LoadLevel(&level0, "Fase 1", 1, &player);

    while (!WindowShouldClose())
    {
        // ATUALIZAÇÕES DO JOGO
        UpdatePlayer(&player, &level0);
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