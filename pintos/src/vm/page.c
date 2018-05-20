#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <debug.h>
#include <inttypes.h>
#include "vm/page.h"
#include "vm/file_map.h"
#include "devices/block.h"
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
			if (spage_entry->mmap)
			{	
				struct file_map * mapped_file = spage_entry->file_map;
				struct list_elem *e;
				for (e = list_begin(&mapped_file->addr); e != list_end(&mapped_file->addr); e = list_next(e))
				{
				    struct addr_elem *addr_elem = list_entry (e, struct addr_elem, elem);
				    if (addr_elem->spage_elem == spage_entry)
				      break;
				}
		  		struct addr_elem *addr_elem = list_entry (e, struct addr_elem, elem);

		  		list_remove(&addr_elem->elem);


			    if (spage_entry->dirty)
			    {
			    	struct block *swap_slot = block_get_role(BLOCK_SWAP);

			    	int i=0;
				    int sector_per_page = PGSIZE / BLOCK_SECTOR_SIZE;

				    int sector = spage_entry->pa;

				    void *tmp = malloc(PGSIZE);

				    for (i = 0 ; i < sector_per_page; i++)
					{
						block_read (swap_slot, sector, ((void *) tmp+BLOCK_SECTOR_SIZE*i));
						sector++;
					}
					acquire_sys_lock();
					file_write_at(mapped_file->file, tmp, PGSIZE, addr_elem->ofs);
			    	release_sys_lock();
					free(tmp);
			    }
			    free(addr_elem);

			    if ( list_empty(&mapped_file->addr))
			    {
			        list_remove(&mapped_file->elem); 
			        free(mapped_file);
		    	}
			}
			swap_remove(spage_entry);
			free(spage_entry);
			break;
		}
		case MMAP:
		{	
			struct file_map * mapped_file = spage_entry->file_map;
			struct list_elem *e;
			for (e = list_begin(&mapped_file->addr); e != list_end(&mapped_file->addr); e = list_next(e)){
			    struct addr_elem *addr_elem = list_entry (e, struct addr_elem, elem);
			    if (addr_elem->spage_elem == spage_entry)
			      break;
			}
		  	struct addr_elem *addr_elem = list_entry (e, struct addr_elem, elem);

		  	list_remove(&addr_elem->elem);
		
		    if (pagedir_is_dirty(mapped_file->t->pagedir, spage_entry->va)||spage_entry->dirty){
		    	acquire_sys_lock();
		    	file_write_at(mapped_file->file, spage_entry->pa, PGSIZE, addr_elem->ofs);
		    	release_sys_lock();
	    	}
	    	frame_remove(spage_entry);

	    	free(addr_elem);
		  
		 	if (list_empty(&mapped_file->addr)){
		        list_remove(&mapped_file->elem); 
		        free(mapped_file);
		    }
		    free(spage_entry);
            break; 
        }
        default:
			break;
	}
}

void 
spage_init(struct hash *page_table)
{
	hash_init(page_table,hash_map,spage_less_func, NULL);
	// lock_init(&hash_lock);
}

bool 
allocate_spage_elem (unsigned va, unsigned pa, enum spage_type flag, void * entry, bool writable)
{
	struct hash *page_table = &thread_current()->supplement_page_table;
	struct spage_entry *fe = malloc(sizeof(struct spage_entry));
	fe->va = va;
	fe->pa = pa;
	fe->pointer = entry;
	fe->page_type = flag;
	fe->writable = writable;
	fe->dirty = false;
	fe->mmap = false;
	fe->file_map = NULL;
	ASSERT(hash_insert(page_table, &fe->elem) == NULL);
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
			hash_delete(page_table, &e);
			free(&f);
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
  acquire_frame_lock();
  acquire_swap_lock();
  hash_destroy(page_table, destory_hash_action);
  release_frame_lock();
  release_swap_lock();
}