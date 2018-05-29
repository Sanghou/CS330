#include "filesys_cache.h"
#include <list.h>

struct list cache;

void cache_init(){

	int i;

	list_init(&cache);

	for(i=0; i<64;i++){
		struct cache_elem cache_unit;
		cache_unit = malloc(sizeof(struct cache_elem));
		list_push_back(&cache, &cache_unit);
	}
}

/*
 
*/

void cache_read(struct block *block, block_secotr_t sector, void * buffer){

}

void cache_write(){

}

void cache_destroy(){
	
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

void cache_write_to_disk(){

}

void cache_evict(){

}

