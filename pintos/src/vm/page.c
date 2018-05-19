#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <debug.h>
#include <inttypes.h>
#include "vm/page.h"
#include "vm/file_map.h"
#include "lib/kernel/hash.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

// struct lock hash_lock;

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
<<<<<<< HEAD
spage_init(struct thread *t)
{
	struct hash *page_table = &t->supplement_page_table;
=======
destory_hash_action(struct hash_elem *e, void *aux)
{
	struct spage_entry *spage_entry = hash_entry(e, struct spage_entry, elem);

	switch (spage_entry->page_type){
		case PHYS_MEMORY:
		{
			frame_remove (spage_entry);
			free(spage_entry);
			break;
		}
		case SWAP_DISK:
		{
			swap_remove(spage_entry);
			free(spage_entry);
			break;
		}
		case MMAP:
		{
			struct file_map * mapped_file = spage_entry->file_map;
			hash_insert(&thread_current()->supplement_page_table, &spage_entry->elem);

          	while(!list_empty(&mapped_file->addr)){

            	struct list_elem* e = list_pop_front(&mapped_file->addr);

           		struct addr_elem *addr_elem = list_entry(e,struct addr_elem, elem);
            	struct spage_entry *se = addr_elem->spage_elem;
           
                struct frame_entry *fe = (struct frame_entry *) se->pointer;

                if(pagedir_is_dirty(thread_current()->pagedir, se->va)){ //check dirty bit
                  acquire_sys_lock();
                  file_write_at(mapped_file->file, fe->frame_number, PGSIZE, addr_elem->ofs);
                  release_sys_lock();
                }
                deallocate_frame_elem(fe->thread, fe->page_number);

                free(addr_elem);
              }
            list_remove(&mapped_file->elem); 
            free(mapped_file);
            break; 
        }
        default:
			break;
	}
}

void 
spage_init(struct hash *page_table)
{
>>>>>>> cd1db56622332dcf7ea7e371d8300c4515869ed4
	hash_init(page_table,hash_map,spage_less_func, NULL);
	// lock_init(&hash_lock);
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
<<<<<<< HEAD
allocate_spage_elem (unsigned va, enum spage_type flag)
=======
allocate_spage_elem (unsigned va, enum spage_type flag, void * entry, bool writable)
>>>>>>> cd1db56622332dcf7ea7e371d8300c4515869ed4
{
	struct hash *page_table = &thread_current()->supplement_page_table;
	struct spage_entry *fe = malloc(sizeof(struct spage_entry));
	fe->va = va;
	fe->page_type = flag;
	fe->writable = writable;
	// lock_acquire(&hash_lock);
	ASSERT(hash_insert(page_table, &fe->elem) == NULL);
	// lock_release(&hash_lock);
}

bool 
deallocate_spage_elem (unsigned va)
{
	struct spage_entry f;
	struct hash_elem *e;
	struct hash *page_table = &thread_current()->supplement_page_table;
	f.va = va; 
	e = hash_find(page_table, &f.elem);
	if (e != NULL)
		{
			// lock_acquire(&hash_lock);
			hash_delete(page_table, &e);
<<<<<<< HEAD
=======
			// lock_release(&hash_lock);
			free(&f);
>>>>>>> cd1db56622332dcf7ea7e371d8300c4515869ed4
			return true;
		}
	return false;
}

struct spage_entry *
mapped_entry (struct thread *t, unsigned va){
	struct spage_entry page_entry;	
	page_entry.va = va;
	struct hash_elem *hash = hash_find(&t->supplement_page_table, &page_entry.elem);


	if(hash == NULL){
	 	return NULL;
	}

	return hash_entry(hash , struct spage_entry, elem);
}

void 
destroy_spage (struct hash *page_table)
{
  hash_destroy(page_table, destory_hash_action);
}