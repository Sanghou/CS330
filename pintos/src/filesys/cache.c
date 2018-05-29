#include "filesys_cache.h"
#include <list.h>

struct list cache;

void cache_init(){
	// int i;
	list_init(&cache);

	// for(i=0; i<64;i++){
	// 	struct cache_elem cache_unit;
	// 	cache_unit = malloc(sizeof(struct cache_elem));
	// 	list_push_back(&cache, &cache_unit);
	// }
}

/*
 
*/

void 
cache_read(struct block *block, block_sector_t sector, void * buffer)
{
	struct list_elem *e;
	for(e = list_begin(&cache); e!=list_end(&cache); e = list_next(e))
	{	
		struct cache_elem *cache = list_entry(e, struct cache_elem, elem);
		if (sector == cache->locate)
		{
			memcpy(buffer, cache->data, 512);
			return;
		}
	}

	cache_read_from_disk(block, sector, buffer);
}

void 
cache_write (struct block *block, block_sector_t sector, void * buffer)
{
	struct list_elem *e;
	for(e = list_begin(&cache); e!=list_end(&cache); e = list_next(e))
	{	
		struct cache_elem *cache = list_entry(e, struct cache_elem, elem);
		if (sector == cache->locate)
		{
			memcpy(cache->data, buffer, 512);
			cache->dirty = 1;
			return;
		}
	}
	block_write (block, sector, buffer);
}

void cache_destroy (){
	
}



void cache_read_from_disk(struct block *block, block_secotr_t sector, void * buffer_){
	

	uint8_t *buffer = buffer_;
  	struct cache_elem cache_unit;

	cache_unit = malloc(sizeof(struct cache_elem));
	if(list_size(&cache) == 64){
		cache_evict();
	}
	/*
	block read, and append data for cache_elem;
	*/
	block_read(block, sector, buffer);
	memcpy(buffer, cache_unit->data, 512);
	cache_unit->dirty = 0;
	cache_unit->locate = sector;
	list_push_back(&cache, &cache_unit);
}

void cache_write_to_disk (){

}

void 
cache_evict (void)
{
	//FIFO
	struct list_elem *elem = list_pop_front(&cache);
	struct cache_elem *cache = list_entry(elem, struct cache_elem, elem);
	if (cache == NULL)
		PANIC("CACHE NOT EXIST\n");

	if (cache->dirty)
		cache_write_to_disk (cache);
	free(cache);
}

