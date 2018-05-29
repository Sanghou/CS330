#include <list.h>
#include <stdio.h>
#include <block.h>


struct cache_elem
{
 block_sector_t sector; //disk location
 bool dirty; // dirty check
 uint8_t data[512];
 struct list_elem elem;
};

void cache_init (void);
struct cache_elem * cache_get(void);
void cache_read(struct block *block, block_sector_t sector, void * buffer);
void cache_write(struct block *block, block_sector_t sector, void * buffer);
void cache_destroy();