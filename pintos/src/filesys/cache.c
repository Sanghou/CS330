#include "filesys_cache.h"
#include <list.h>

struct list cache;

void cache_init(){
	list_init(&cache);
}

/*
 
*/

void cache_read(struct block *block, block_secotr_t sector, void * buffer){

}

void cache_write(){

}

void cache_destroy(){

	int i;
	unsigned list_size = list_size(&cache);
	struct cache_elem cache_unit;

	for(i=list_size; i>0; i--){
		cache_unit = list_pop_front(&cache);
		if(cache_unit.dirty ==1){
			cache_write_to_disk(cache_unit);
		}
	}
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
	memcpy(cache_unit->data, buffer, 512);
	cache_unit->dirty = 0;
	cache_unit->locate = sector;
	list_push_back(&cache, &cache_unit);
}

/*
use for evict and cache_destroy
*/

void cache_write_to_disk(struct cache_elem * cache_unit){
	struct block * block = block_get_by_name("filesys");
	block_write(block, cache_unit->sector,cache_unit->data);
}

void cache_evict(){

}	

