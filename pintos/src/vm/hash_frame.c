#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <debug.h>
#include <inttypes.h>
#include "vm/hash_frame.h"
#include "lib/kernel/hash.h"
#include "threads/palloc.h"

static struct hash *frame_table;


bool frame_less_func (const struct hash_elem *a,
                             const struct hash_elem *b,
                             void *aux){
	return true;
}

unsigned hash_map(const struct hash_elem *e, void *aux){
	const struct frame_entry *p = hash_entry(e, struct frame_entry, hash_elem); ////
	return hash_int(p->pa>>12);
}

void frame_init(){
	frame_table = palloc_get_page(PAL_ASSERT);
	hash_init(frame_table,hash_map,frame_less_func, NULL);
}

bool allocate_frame_elem(unsigned pa, unsigned va){
	struct frame_entry *fe = malloc(sizeof(struct frame_entry));
	fe->pa = pa;
	fe->va = va;
	fe->evict = 0;
	ASSERT(hash_insert(frame_table, &fe->hash_elem) == NULL);
}

bool deallocate_frame_elem(unsigned pa){
	struct frame_entry f;
	struct hash_elem *e;
	f.pa = pa; 
	e = hash_find(frame_table, &f.hash_elem);
	if(e != NULL){
		hash_delete(frame_table, &e);
		return true;
	}
	return false;

}
struct frame_entry* evict(){
	return NULL;
}
