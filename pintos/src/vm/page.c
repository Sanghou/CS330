#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <debug.h>
#include <inttypes.h>
#include "vm/page.h"
#include "lib/kernel/hash.h"
#include "threads/palloc.h"

static struct hash *page_table;


bool spage_less_func (const struct hash_elem *a,
                             const struct hash_elem *b,
                             void *aux){
	return true;
}

unsigned hash_map(const struct hash_elem *e, void *aux){
	const struct spage_entry *p = hash_entry(e, struct spage_entry, hash_elem); ////
	return hash_int(p->page_number>>12);
}

void spage_init(){
	page_table = palloc_get_page(PAL_ASSERT);
	hash_init(page_table,hash_map,spage_less_func, NULL);
}

bool allocate_spage_elem(unsigned pa, unsigned va){
	struct spage_entry *fe = malloc(sizeof(struct spage_entry));
	fe->page_number = pa;
	fe->va = va;
	ASSERT(hash_insert(page_table, &fe->hash_elem) == NULL);
}

bool deallocate_frame_elem(unsigned pa){
	struct spage_entry f;
	struct hash_elem *e;
	f.pa = pa; 
	e = hash_find(page_table, &f.hash_elem);
	if(e != NULL){
		hash_delete(page_table, &e);
		return true;
	}
	return false;
}
