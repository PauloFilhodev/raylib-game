#include "raylib.h"
#include "raymath.h"
#include <stddef.h>

#include "blocks.h"
#include "assets.h"

// Tamanho do sprite em pixels (a imagem original tem 40x40)
#define BLOCK_SPRITE_SIZE 44.0f

static Vector2 SlotCenter(const MemorySlot *slot)
{
    return (Vector2){
        slot->pixelPosition.x + TILE_SIZE / 2.0f,
        slot->pixelPosition.y + TILE_SIZE / 2.0f};
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
    // Atordoado: nao consegue pegar nem soltar
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

void DrawMemBlocks(const Level *lvl)
{
    Texture2D t = gAssets.memBlock;

    for (int i = 0; i < lvl->blockCount; i++)
    {
        const MemoryBlock *m = &lvl->memoryBlock[i];
        int x = (int)m->pixelPosition.x;
        int y = (int)m->pixelPosition.y;

        if (t.id == 0)
        {
            // Sem textura: círculo colorido como antes
            DrawCircle(x, y, m->radius, m->blockColor);
            DrawCircleLines(x, y, m->radius, BLACK);
        }
        else
        {
            // Mesmo sprite para todos os blocos; o tamanho vem do raio.
            // Quer tingir pela cor do valor? Troque WHITE por m->blockColor.
            float size = BLOCK_SPRITE_SIZE;

            Rectangle src = {0, 0, (float)t.width, (float)t.height};
            Rectangle dst = {m->pixelPosition.x, m->pixelPosition.y, size, size};
            Vector2 origin = {size / 2.0f, size / 2.0f};

            DrawTexturePro(t, src, dst, origin, 0.0f, WHITE);
        }

        // Texto acima do bloco: indica a qual slot ele pertence (mesmo numero do slot)
        const char *txt = TextFormat("%d", m->blockValue);
        int w = MeasureText(txt, 18);
        int tx = x - w / 2;
        int ty = y - (int)(BLOCK_SPRITE_SIZE / 2.0f) - 20;
        DrawText(txt, tx + 1, ty + 1, 18, BLACK); // sombra
        DrawText(txt, tx, ty, 18, WHITE);
    }
}

void DrawMemSlots(const Level *lvl)
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
