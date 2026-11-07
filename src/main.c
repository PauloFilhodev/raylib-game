#include "raylib.h"
#include "raymath.h"
#include "game_types.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#include "player.c"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

static Vector2 SlotCenter(const MemorySlot *slot)
{
    return (Vector2){
        slot->pixelPosition.x + TILE_SIZE / 2.0f,
        slot->pixelPosition.y + TILE_SIZE / 2.0f};
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