#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <debug.h>
#include <inttypes.h>
#include "lib/kernel/hash.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "vm/page.h"



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
spage_init(struct thread *t)
{
	struct hash *page_table = &t->supplement_page_table;
	hash_init(page_table,hash_map,spage_less_func, NULL);
}

void
element_destroy(struct hash_elem *e, void *aux)
{
	//***************
	// to do destroy : hash entry return type -> 
	// save kpage.

	// not swapped -> palloc free


	// if mmap, dirty : write back.
	// dirty check : pagedir_is_dirty();

	// else -> just die.



}

void
spage_destroy(struct hash *spt)
{
	hash_destroy(spt, element_destroy);
}


bool 
allocate_spage_elem (unsigned va, enum spage_type flag)
{
	struct hash *page_table = thread_current()->supplement_page_table;
	struct spage_entry *fe = malloc(sizeof(struct spage_entry));
	struct thread* t = thread_current();
	fe->va = va;
	fe->page_type = flag;
	ASSERT(hash_insert(page_table, &fe->elem) == NULL);
}

bool 
deallocate_spage_elem (unsigned va)
{
	struct spage_entry f;
	struct hash_elem *e;
	struct hash *page_table = thread_current()->supplement_page_table;
	f.va = va; 
	e = hash_find(page_table, &f.elem);
	if (e != NULL)
		{
			hash_delete(page_table, &e);
			return true;
		}
	return false;
}

struct spage_entry *
mapped_entry (struct thread *t, unsigned va){
	struct spage_entry *page_entry;	
	page_entry->va = va;
	struct hash_elem *hash = hash_find(&thread_current()->supplement_page_table, &page_entry->elem);

	if(hash == NULL){
	 	return NULL;
	}

	return hash_entry(hash , struct spage_entry, elem);
}