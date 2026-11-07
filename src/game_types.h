
#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <raylib.h>
#include <stdbool.h>

#define MAP_ROWS 18
#define MAP_COLS 32
#define TILE_SIZE 40

#define MAX_MEM_BLOCKS 10
#define MAX_SLOT_BLOCKS 10

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
    TILE_SPAWN_PLAYER = 2,
    TILE_SPAWN_BLOCK = 3,
    TILE_SLOT_RAM = 4
} TileType;

// LEVEL
typedef struct Level
{
    int levelNumber;
    int blockCount, blockSlots;
    const char *levelName;
    float timeLimit;

    int tileMap[MAP_ROWS][MAP_COLS];

    MemoryBlock memoryBlock[MAX_MEM_BLOCKS];
    MemorySlot memorySlot[MAX_SLOT_BLOCKS];

    Vector2 spawnPlayer;
    Vector2 spawnMemoryBlock;
    Vector2 spawnMemorySlot;
} Level;

// PLAYER
typedef struct Player
{
    Vector2 playerPosition;
    float speed;
    float radius;

    bool isHoldingBlock;
    MemoryBlock *carryingBlock;
} Player;

#endif