#include "filesys_cache.h"
#include <list.h>

struct list cache;

void cache_init(){

	int i;

	list_init(&cache);

	for(i=0; i<64;i++){
		struct cache_elem cache_unit;
		cache_unit = malloc(sizeof(struct cache_elem));
		cache_unit.valid = 0;
		list_push_back(&cache, &cache_unit);
	}
}

void cache_read_from_disk(){
	
}

void cache_write_to_disk(){

}

void cache_evict(){

}

struct cache_elem * cache_read_to_memory(){

}

void cache_remove(){
	
}

