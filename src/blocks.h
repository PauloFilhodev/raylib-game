#ifndef BLOCKS_H
#define BLOCKS_H

#include "game_types.h"

void FitMemoryBlock(MemoryBlock *block, Level *lvl);
void PickUpOrReleaseBlock(Player *p, Level *lvl);
void DrawMemBlocks(const Level *lvl);
void DrawMemSlots(const Level *lvl);

#endif
