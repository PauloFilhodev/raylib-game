#include "raylib.h"
#include "raymath.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "game_types.h"
#include "levels.h"
#include "player.h"

#define WINDOW_WIDTH (MAP_COLS * TILE_SIZE)  // 1280
#define WINDOW_HEIGHT (MAP_ROWS * TILE_SIZE) // 720

typedef enum
{
    STATE_PLAYING,
    STATE_GAMEOVER,
    STATE_WIN,
    STATE_ERROR
} GameState;

static const char *LevelPath(int number)
{
    static char path[64];
    snprintf(path, sizeof path, "levels/level%d.txt", number);
    return path;
}

static Vector2 SlotCenter(const MemorySlot *slot)
{
    return (Vector2){
        slot->pixelPosition.x + TILE_SIZE / 2.0f,
        slot->pixelPosition.y + TILE_SIZE / 2.0f};
}

// ---------------------------------------------------------------------------------------------
// CARREGAMENTO DO LEVEL

static bool StartLevel(int number, Level *lvl, Player *p, float *timeLeft)
{
    if (!LoadLevel(LevelPath(number), lvl))
        return false;

    lvl->levelNumber = number;
    InitPlayer(p, lvl->spawnPlayer);
    *timeLeft = lvl->timeLimit;

    return true;
}

// ---------------------------------------------------------------------------------------------
// BLOCOS E SLOTS

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
static void FitMemoryBlock(MemoryBlock *block, Level *lvl)
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

static void PickUpOrReleaseBlock(Player *p, Level *lvl)
{
    // Atordoado: não consegue pegar nem soltar
    if (p->stunTimer <= 0.0f && IsKeyPressed(KEY_SPACE))
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
// DESENHO

static void DrawMemBlock(const Level *lvl)
{
    for (int i = 0; i < lvl->blockCount; i++)
    {
        const MemoryBlock *m = &lvl->memoryBlock[i];
        int x = (int)m->pixelPosition.x;
        int y = (int)m->pixelPosition.y;

        DrawCircle(x, y, m->radius, m->blockColor);
        DrawCircleLines(x, y, m->radius, BLACK);

        const char *txt = TextFormat("%d", m->blockValue);
        DrawText(txt, x - MeasureText(txt, 16) / 2, y - 8, 16, WHITE);
    }
}

static void DrawMemSlot(const Level *lvl)
{
    for (int i = 0; i < lvl->blockSlots; i++)
    {
        const MemorySlot *s = &lvl->memorySlot[i];
        int x = (int)s->pixelPosition.x;
        int y = (int)s->pixelPosition.y;

        // Retângulo primeiro, texto depois
        DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, Fade(s->slotColor, s->isFilled ? 0.9f : 0.6f));
        DrawRectangleLines(x, y, TILE_SIZE, TILE_SIZE, WHITE);
        DrawText(TextFormat("%d", s->slotValue), x + 4, y + 2, 16, WHITE);
    }
}

static void DrawCenteredText(const char *text, int y, int size, Color color)
{
    DrawText(text, (WINDOW_WIDTH - MeasureText(text, size)) / 2, y, size, color);
}

// Obs.: a fonte padrão da raylib não tem acentos, por isso os textos da tela são sem acento.
static void DrawHUD(const Level *lvl, const Player *p, float timeLeft)
{
    // Linha de cima (parede)
    DrawText(TextFormat("Fase %d - %s", lvl->levelNumber, lvl->levelName), 12, 10, 20, WHITE);
    DrawText(TextFormat("Slots: %d/%d", CountFilledSlots(lvl), lvl->blockSlots), 520, 10, 20, WHITE);
    DrawText(TextFormat("Tempo: %.1f", timeLeft), 1090, 10, 20, timeLeft <= 10.0f ? RED : WHITE);

    // Linha de baixo (parede)
    int y = WINDOW_HEIGHT - 30;

    if (p->stunTimer > 0.0f)
        DrawText(TextFormat("ATORDOADO! %.1fs", p->stunTimer), 12, y, 20, YELLOW);
    if (p->reverseTimer > 0.0f)
        DrawText(TextFormat("CONTROLES INVERTIDOS! %.1fs", p->reverseTimer), 260, y, 20, MAGENTA);

    DrawText("WASD: mover | ESPACO: pegar/soltar", 840, y, 20, LIGHTGRAY);
}

// ---------------------------------------------------------------------------------------------

int main(void)
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Byte Bound");
    SetTargetFPS(60);

    Player player = {0};
    Level level = {0};
    int currentLevel = 1;
    float timeLeft = 0.0f;
    GameState state = STATE_PLAYING;
    const char *endReason = "";
    bool hasNextLevel = false;

    if (!StartLevel(currentLevel, &level, &player, &timeLeft))
        state = STATE_ERROR;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // ATUALIZAÇÕES DO JOGO
        switch (state)
        {
        case STATE_PLAYING:
            timeLeft -= dt;

            UpdatePlayer(&player, &level, dt);
            UpdateEnemies(&level, dt);
            PickUpOrReleaseBlock(&player, &level);

            if (CheckEnemyCollisions(&player, &level))
            {
                endReason = "VIRUS DETECTADO! Voce foi infectado.";
                state = STATE_GAMEOVER;
            }
            else if (timeLeft <= 0.0f)
            {
                timeLeft = 0.0f;
                endReason = "SYSTEM FREEZE! O tempo acabou.";
                state = STATE_GAMEOVER;
            }
            else if (CountFilledSlots(&level) == level.blockSlots)
            {
                hasNextLevel = FileExists(LevelPath(currentLevel + 1));
                state = STATE_WIN;
            }
            break;

        case STATE_GAMEOVER:
            if (IsKeyPressed(KEY_R))
                state = StartLevel(currentLevel, &level, &player, &timeLeft) ? STATE_PLAYING : STATE_ERROR;
            break;

        case STATE_WIN:
            if (hasNextLevel && IsKeyPressed(KEY_ENTER))
            {
                currentLevel++;
                state = StartLevel(currentLevel, &level, &player, &timeLeft) ? STATE_PLAYING : STATE_ERROR;
            }
            break;

        case STATE_ERROR:
            break;
        }

        // RENDERIZAÇÃO DO JOGO
        BeginDrawing();
        ClearBackground(BLACK);

        if (state == STATE_ERROR)
        {
            DrawCenteredText("ERRO AO CARREGAR A FASE", 270, 40, RED);
            DrawCenteredText(TextFormat("Arquivo: %s", LevelPath(currentLevel)), 340, 24, WHITE);
            DrawCenteredText("Veja o terminal para saber a linha do erro.", 380, 20, LIGHTGRAY);
            DrawCenteredText("Rode o jogo a partir da pasta que contem 'levels/'.", 410, 20, LIGHTGRAY);
        }
        else
        {
            DrawLevel(&level);
            DrawMemSlot(&level);
            DrawEnemies(&level);
            DrawPlayer(&player);
            DrawMemBlock(&level); // depois do player: o bloco carregado fica visível
            DrawHUD(&level, &player, timeLeft);

            if (state == STATE_GAMEOVER)
            {
                DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, Fade(BLACK, 0.7f));
                DrawCenteredText("GAME OVER", 260, 60, RED);
                DrawCenteredText(endReason, 340, 24, WHITE);
                DrawCenteredText("Pressione R para tentar de novo", 390, 20, LIGHTGRAY);
            }
            else if (state == STATE_WIN)
            {
                DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, Fade(BLACK, 0.7f));
                DrawCenteredText("FASE CONCLUIDA!", 260, 60, GREEN);
                DrawCenteredText(TextFormat("Tempo restante: %.1fs", timeLeft), 340, 24, WHITE);
                if (hasNextLevel)
                    DrawCenteredText("Pressione ENTER para a proxima fase", 390, 20, LIGHTGRAY);
                else
                    DrawCenteredText("Voce terminou todas as fases! (ESC para sair)", 390, 20, LIGHTGRAY);
            }
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}