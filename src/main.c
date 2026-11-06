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

#define ENEMY_COUNT 6

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
    float health;
    float hitcooldown;

    // Referente ao que ele segura
    bool isHoldingBlock;
    MemoryBlock *carryingBlock;
} Player;

// VARIAVEIS DO INIMIGO
typedef struct Enemy
{
    Vector2 enemyPosition;
    float speed;
    float radius;
    float damage;

    Rectangle area; // área onde o inimigo pode andar
    Vector2 target; // ponto atual para onde ele está indo
} Enemy;

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

// Sorteia um ponto dentro da área do inimigo, respeitando o raio dele
static Vector2 RandomPointInArea(const Enemy *e)
{
    return (Vector2){
        GetRandomValue((int)(e->area.x + e->radius), (int)(e->area.x + e->area.width - e->radius)),
        GetRandomValue((int)(e->area.y + e->radius), (int)(e->area.y + e->area.height - e->radius))};
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

void ResolveWallCollisionsCircle(Vector2 *position, float radius, const Level *lvl)
{
    for (int pass = 0; pass < 4; pass++)
    {
        Vector2 pos = *position;

        int colStart = (int)floorf((pos.x - radius) / TILE_SIZE);
        int colEnd = (int)floorf((pos.x + radius) / TILE_SIZE);
        int rowStart = (int)floorf((pos.y - radius) / TILE_SIZE);
        int rowEnd = (int)floorf((pos.y + radius) / TILE_SIZE);

        bool found = false;
        float bestDist = radius;
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

                if (dist >= radius)
                    continue;

                Vector2 push;

                if (dist > 0.0001f)
                {
                    push = Vector2Scale(diff, (radius - dist) / dist);
                }
                else
                {
                    float dl = pos.x - wall.x;
                    float dr = wall.x + wall.width - pos.x;
                    float dt = pos.y - wall.y;
                    float db = wall.y + wall.height - pos.y;
                    float m = fminf(fminf(dl, dr), fminf(dt, db));

                    if (m == dl)
                        push = (Vector2){-(dl + radius), 0.0f};
                    else if (m == dr)
                        push = (Vector2){dr + radius, 0.0f};
                    else if (m == dt)
                        push = (Vector2){0.0f, -(dt + radius)};
                    else
                        push = (Vector2){0.0f, db + radius};
                }

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

        *position = Vector2Add(*position, bestPush);
    }
}

// ---------------------------------------------------------------------------------------------
// FUNÇÕES DE INICIO
void InitPlayer(Player *p, Vector2 position)
{
    p->playerPosition = position;
    p->radius = 15.0f;
    p->speed = 200.0f;
    p->health = 100.0f;
    p->hitcooldown = 0.0f;
    p->isHoldingBlock = false;
    p->carryingBlock = NULL;
}

void InitEnemy(Enemy *e, Vector2 position, Rectangle area)
{
    e->enemyPosition = position;
    e->radius = 15.0f;
    e->speed = 100.0f;
    e->damage = 20.0f;
    e->area = area;
    e->target = RandomPointInArea(e);
}

// ---------------------------------------------------------------------------------------------
// FUNÇÕES DE ATUALIZAÇÃO
void UpdatePlayer(Player *p, Level *lvl)
{
    Vector2 direction = {0.0f, 0.0f};

    // Tempo de invulnerabilidade após levar dano
    if (p->hitcooldown > 0.0f)
        p->hitcooldown -= GetFrameTime();

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

void UpdateEnemy(Enemy *e, Player *p, const Level *lvl)
{
    float dt = GetFrameTime();

    // PATRULHAR dentro da área
    Vector2 direction = Vector2Subtract(e->target, e->enemyPosition);

    if (Vector2Length(direction) < 5.0f)
    {
        e->target = RandomPointInArea(e); // chegou: escolhe novo ponto
    }
    else
    {
        direction = Vector2Normalize(direction);
        e->enemyPosition.x += direction.x * e->speed * dt;
        e->enemyPosition.y += direction.y * e->speed * dt;
    }

    ResolveWallCollisionsCircle(&e->enemyPosition, e->radius, lvl);

    // ATACAR (dano por contato)
    float distance = Vector2Distance(e->enemyPosition, p->playerPosition);

    if (distance <= e->radius + p->radius && p->hitcooldown <= 0.0f)
    {
        p->health -= e->damage;
        if (p->health < 0.0f)
            p->health = 0.0f;
        p->hitcooldown = 1.0f; // 1 segundo sem poder levar dano de novo
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

void DrawEnemy(const Enemy *e)
{
    DrawCircle((int)e->enemyPosition.x, (int)e->enemyPosition.y, e->radius, WHITE);
    DrawCircleLines((int)e->enemyPosition.x, (int)e->enemyPosition.y, e->radius, BLACK);
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
    Enemy enemies[ENEMY_COUNT] = {0};
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
    // Áreas dos inimigos (em pixels: x, y, largura, altura)
    // Inimigo 0: colunas 22–29, linhas 1–4
    InitEnemy(&enemies[0],
              (Vector2){26 * TILE_SIZE, 3 * TILE_SIZE},
              (Rectangle){22 * TILE_SIZE, 1 * TILE_SIZE, 8 * TILE_SIZE, 4 * TILE_SIZE});
    // Inimigo 1: colunas 2–10, linhas 14–16
    InitEnemy(&enemies[1],
              (Vector2){6 * TILE_SIZE, 15 * TILE_SIZE},
              (Rectangle){2 * TILE_SIZE, 14 * TILE_SIZE, 9 * TILE_SIZE, 3 * TILE_SIZE});
    // Inimigo 2: colunas 20–29, linhas 6–9
    InitEnemy(&enemies[2],
              (Vector2){25 * TILE_SIZE, 8 * TILE_SIZE},
              (Rectangle){20 * TILE_SIZE, 6 * TILE_SIZE, 10 * TILE_SIZE, 4 * TILE_SIZE});
    // Inimigo 3: colunas 14–22, linhas 14–16
    InitEnemy(&enemies[3],
              (Vector2){18 * TILE_SIZE, 15 * TILE_SIZE},
              (Rectangle){14 * TILE_SIZE, 14 * TILE_SIZE, 9 * TILE_SIZE, 3 * TILE_SIZE});
    // Inimigo 4: colunas 1–8, linhas 4–10 
    InitEnemy(&enemies[4],
              (Vector2){5 * TILE_SIZE, 7 * TILE_SIZE},
              (Rectangle){1 * TILE_SIZE, 4 * TILE_SIZE, 8 * TILE_SIZE, 7 * TILE_SIZE});
    // Inimigo 5: colunas 10–20, linhas 1–5 
    InitEnemy(&enemies[5],
              (Vector2){15 * TILE_SIZE, 3 * TILE_SIZE},
              (Rectangle){10 * TILE_SIZE, 1 * TILE_SIZE, 11 * TILE_SIZE, 5 * TILE_SIZE});
    LoadLevel(&level0, "Fase 1", 1, &player);

    while (!WindowShouldClose())
    {
        // ATUALIZAÇÕES DO JOGO
        UpdatePlayer(&player, &level0);
        PickUpOrReleaseBlock(&player, &level0);
        for (int i = 0; i < ENEMY_COUNT; i++)
            UpdateEnemy(&enemies[i], &player, &level0);

        // RENDERIZAÇÃO DO JOGO
        BeginDrawing();

        ClearBackground(RAYWHITE);
        // FUNÇÕES DRAW
        DrawLevel(&level0);
        DrawMemSlot(&level0);
        DrawMemBlock(&level0);
        for (int i = 0; i < ENEMY_COUNT; i++)
            DrawEnemy(&enemies[i]);
        DrawPlayer(&player);
        // INTERFACE
        DrawText(level0.levelName, 0, 0, 20, RED);
        DrawText(TextFormat("Health: %.0f", player.health), 0, 40, 20, RED);
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