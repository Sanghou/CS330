#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <debug.h>
#include <inttypes.h>
#include "vm/page.h"
#include "lib/kernel/hash.h"
#include "threads/palloc.h"
#include "threads/thread.h"



bool 
spage_less_func	(const struct hash_elem *a,
                 const struct hash_elem *b, void *aux)
{
	struct spage_entry *pa = hash_entry(a, struct spage_entry, elem); 	
	struct spage_entry *pb = hash_entry(b, struct spage_entry, elem); 	

	if (pa->va < pb->va)
		return true;
	return false;
}

unsigned 
hash_map (const struct hash_elem *e, void *aux)
{
	const struct spage_entry *p = hash_entry(e, struct spage_entry, elem); ////
	return hash_int(p->va>>12);
}

void 
spage_init()
{
	struct hash *page_table = thread_current()->supplement_page_table;
		page_table = palloc_get_page(PAL_ASSERT);
	hash_init(page_table,hash_map,spage_less_func, NULL);
}

bool 
allocate_spage_elem (unsigned pa, unsigned va)
{
	struct hash *page_table = thread_current()->supplement_page_table;
	struct spage_entry *fe = malloc(sizeof(struct spage_entry));
	struct thread* t = thread_current();
	fe->va = va;
	fe->pa = pa;
	fe->thread = t;
	fe->evict = 0;
	ASSERT(hash_insert(page_table, &fe->elem) == NULL);
}

bool 
deallocate_spage_elem (unsigned pa)
{
	struct spage_entry f;
	struct hash_elem *e;
	struct hash *page_table = thread_current()->supplement_page_table;
	f.pa = pa; 
	e = hash_find(page_table, &f.elem);
	if (e != NULL)
		{
			hash_delete(page_table, &e);
			free(&f);
			return true;
		}
	return false;
}

struct spage_entry *
mapped_entry (struct thread *t, unsigned va){
	struct spage_entry *page_entry;	
	page_entry->va = va;
	struct hash_elem *hash = hash_find(&t->supplement_page_table, &page_entry->elem);

	if(hash == NULL){
	 	return NULL;
	}

	return hash_entry(hash , struct spage_entry, elem);
}