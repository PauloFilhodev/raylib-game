#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <raylib.h>
#include <stdbool.h>

#define MAP_ROWS 18
#define MAP_COLS 32
#define TILE_SIZE 40

#define MAX_MEM_BLOCKS 10
#define MAX_SLOT_BLOCKS 10
#define MAX_ENEMIES 10 // ids de 0 a 9 no arquivo do mapa

// BLOCO DE MEMÓRIA
typedef struct MemoryBlock
{
    bool isCarried;
    bool isPlaced;

    Rectangle hitbox;

    int gridX, gridY;
    Vector2 pixelPosition, spawnPosition;

    float radius;
    Color blockColor;

    int blockValue;
} MemoryBlock;

// SLOT DE MEMÓRIA
typedef struct MemorySlot
{
    bool isFilled;

    Rectangle hitbox;

    int gridX, gridY;
    Vector2 pixelPosition, spawnPosition;

    Color slotColor;

    int slotValue;
} MemorySlot;

// TIPOS DE TILE
typedef enum
{
    TILE_FLOOR = 0,
    TILE_WALL = 1,
    TILE_BUS = 2 // trilha de barramento (por enquanto só visual)
} TileType;

// INIMIGOS
typedef enum
{
    ENEMY_KILL,   // 'K': perde a fase
    ENEMY_STUN,   // 'S': atordoa o jogador
    ENEMY_REVERSE // 'R': inverte os controles
} EnemyType;

typedef struct Enemy
{
    EnemyType type;
    Vector2 a, b; // extremos da patrulha (pixels, centro do tile)
    Vector2 pos;  // posição atual
    float speed;  // pixels/segundo
    float half;   // metade do lado do quadrado (hitbox)
    bool toB;     // true = indo para B
} Enemy;

// LEVEL
typedef struct Level
{
    int levelNumber;
    char levelName[64];
    float timeLimit;

    int tileMap[MAP_ROWS][MAP_COLS];

    Vector2 spawnPlayer;

    int blockCount, blockSlots;
    MemoryBlock memoryBlock[MAX_MEM_BLOCKS];
    MemorySlot memorySlot[MAX_SLOT_BLOCKS];

    int enemyCount;
    Enemy enemy[MAX_ENEMIES];
} Level;

// PLAYER
typedef struct Player
{
    Vector2 playerPosition;
    float speed;
    float radius;

    bool isHoldingBlock;
    MemoryBlock *carryingBlock;

    float stunTimer;    // > 0: atordoado (não se move nem pega/solta)
    float reverseTimer; // > 0: controles invertidos
    float immuneTimer;  // > 0: não sofre stun/reverse de novo (evita repetir a cada frame)
} Player;

#endif