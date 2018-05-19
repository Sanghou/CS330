#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
#include <lib/kernel/bitmap.h>
#include <lib/kernel/hash.h>
#include "devices/block.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "threads/pte.h"
#include "userprog/pagedir.h"
#include "page_table.h"


static struct list swap_table;
static struct lock swap_lock;
static struct semaphore sema;
static struct bitmap *used_sector;



static struct list page_table;
static struct lock frame_lock;
static struct list_elem *pointer;



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
			break;
		}

	}

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
	struct hash page_table = thread_current()->supplement_page_table;
	struct spage_entry *fe = malloc(sizeof(struct spage_entry));
	struct thread* t = thread_current();
	fe->va = va;
	fe->page_type = flag;
	ASSERT(hash_insert(&	page_table, &fe->elem) == NULL);
}

bool 
deallocate_spage_elem (unsigned va)
{
	struct spage_entry f;
	struct hash_elem *e;
	struct hash page_table = thread_current()->supplement_page_table;
	f.va = va; 
	e = hash_find(&page_table, &f.elem);
	if (e != NULL)
		{
			hash_delete(&page_table, &e);
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




/*
   Creates a swap table list
   which manages swap slots.
*/

void 
swap_list_init (void){
	list_init(&swap_table);
	lock_init(&swap_lock);
	sema_init(&sema, 1);

	struct block * block = block_get_role(BLOCK_SWAP);
	block_sector_t sector_size = block_size(block);
	used_sector = bitmap_create(sector_size);
}

/*
   Writes frame information in swap disk.
   Manage the swap_table list
*/
//*****************

//swap out, in , supplement page table entry flag change.



// when swapping, kpage and vpage changing save properly.



bool 
swap_in (struct thread *t, unsigned page_num){
	//find the proper swap slot in the swap_table
	struct list_elem *e;
	struct block *swap_slot = block_get_role(BLOCK_SWAP);

	// struct block *swap_slot = block_get_role(BLOCK_SWAP);
	for (e = list_begin(&swap_table); e != list_end(&swap_table); e = list_next(e))
	  {
	    struct swap_entry *se = list_entry(e, struct swap_entry, list_elem);

	    if (t == se->thread && pg_no(se->page_number) == pg_no(page_num))
	      {
	      	struct frame_entry* fe = allocate_frame_elem(se->page_number, se->writable);

	      	// allocate_spage_elem(t->page_number, t->frame_number);
	      	bool success = pagedir_set_page(t->pagedir, (void *) fe->page_number, (void *) fe->frame_number, true);
	      	if (!success) 
	      	{
	      		palloc_free_page((void *)fe->frame_number);
	      		return false;
	      	}

	      	list_remove(e);	

	      	int i;
			int sector_per_page = PGSIZE / BLOCK_SECTOR_SIZE;
			int sector = se->sector;

	      	for (i = 0 ; i < sector_per_page; i++)
			{
				void * tmp = malloc(BLOCK_SECTOR_SIZE);
				// acquire_sys_lock();
				block_read (swap_slot, sector, tmp);
				// release_sys_lock();
				memcpy(((void *) fe->frame_number+BLOCK_SECTOR_SIZE*i), tmp, BLOCK_SECTOR_SIZE);
	      		free(tmp);
				sector++;
			}

			lock_acquire(&swap_lock);
			bitmap_set_multiple (used_sector, se->sector, sector_per_page, false);
			lock_release(&swap_lock);
	   
	      	free(se);
	        return true;
	      }
	  }
	  //printf("cannot find swap \n");
	return false;
}

/*
   Writes frame information in swap disk.
   Manage the swap_table list
   returns false when swap table is full.
*/
void 
swap_out (struct frame_entry *frame)
{
	//printf("swap out! %p %p\n", frame->frame_number, frame->page_number);
	//swap block initialize in where?
	struct swap_entry *se;

	struct block *swap_slot = block_get_role(BLOCK_SWAP);

	int sector_number;
	int sector_per_page = PGSIZE / BLOCK_SECTOR_SIZE;

	int i;

	se = malloc(sizeof(struct swap_entry));

	if (swap_slot == NULL)
		{
			//swap_slot is full
			PANIC("CANNOT FIND SWAP DISK");
		}

	se->page_number = frame->page_number; 		
	se->thread = frame->thread;
	se->writable = frame->writable;

	//여기 block
	void * paddr = (frame->frame_number);

	lock_acquire(&swap_lock);
	size_t sector = bitmap_scan_and_flip(used_sector, 0, sector_per_page, false);
	lock_release(&swap_lock);
	se->sector = sector; 

	if (sector == BITMAP_ERROR)
		PANIC("SWAP BLOCK IS FULL");

	//write to a swap block.
	for (i = 0 ; i < sector_per_page; i++)
	{
		// acquire_sys_lock();
		paddr = ((void *) frame->frame_number) + BLOCK_SECTOR_SIZE * i;
		block_write(swap_slot, sector, paddr);
		// release_sys_lock();
		sector++;
	}
	
	list_push_back(&swap_table, &se->list_elem);
	
	// how to manage the supplementary page table?
}

void
swap_remove (struct spage_entry *spage_entry)
{
	struct swap_entry *se = (struct swap_entry *) spage_entry->pointer;
	
    int sector_per_page = PGSIZE / BLOCK_SECTOR_SIZE;

	list_remove(&se->list_elem);

	lock_acquire(&swap_lock);
	bitmap_set_multiple (used_sector, se->sector, sector_per_page, false);
	lock_release(&swap_lock);

	free(se);

}




/*
void 
swap_remove (struct thread *t)
{
	struct list_elem *e;
	bool success = false;

	for (e = list_begin(&swap_table); e != list_end(&swap_table); e = list_next(e))
	  {
	    struct swap_entry *se = list_entry(e, struct swap_entry, list_elem);

	    if (success){
	    	struct swap_entry *se = list_entry(list_prev(e), struct swap_entry, list_elem);
	    	list_remove(list_prev(e));
	    	free(se);
	    }
	    success = false;

	    if (t == se->thread)
	      {
	      	success = true;

			int sector_per_page = PGSIZE / BLOCK_SECTOR_SIZE;
			int sector = se->sector;

			lock_acquire(&swap_lock);
			bitmap_set_multiple (used_sector, se->sector, sector_per_page, false);
			lock_release(&swap_lock);

	      	// free(se);
	      }
	  }
	
}
*/




void pointer_set (void);


void 
frame_init (void)
{
	list_init(&page_table);
	lock_init(&frame_lock);
	pointer = NULL;
}


struct frame_entry * allocate_frame_elem(uint8_t *upage, bool writable){
	
	uint8_t *kpage = palloc_get_page (PAL_USER);
	
	if(kpage == NULL){
		kpage = evict();
	}
	
	struct frame_entry *fe;
	
	fe = malloc(sizeof(struct frame_entry));
	fe->thread = thread_current();
	fe->page_number = upage;
	fe->frame_number = kpage;
	fe->evict = 1;
	fe->writable = writable;
	lock_acquire(&frame_lock);
	list_push_back(&page_table, &fe->elem);	
	lock_release(&frame_lock);
	pointer_set();

	return fe;
}


bool deallocate_frame_elem(unsigned pn){
	struct frame_entry *f;
	struct list_elem *e;

	lock_acquire(&frame_lock);
	for (e = list_begin(&page_table); e != list_end(&page_table); e = list_next(e))
	  {
    	f = list_entry(e, struct frame_entry, elem);
    	if (f->page_number == pn)
    	  {
    	  	if (pointer == e)
    	  	{
    	  		pointer = list_next(e);
    	  		if (pointer == list_end(&page_table))
    	  			pointer = list_begin(&page_table);
    	  	}

    		pagedir_clear_page(f->thread->pagedir, f->page_number);
    		palloc_free_page((void *)(f->frame_number));
    		free(f);

    		list_remove(e);
    		lock_release(&frame_lock);
    		return true;
    	  }
	  }
	lock_release(&frame_lock);
	return false;
}

uint8_t*
evict (void) // 2-chance
{

	struct frame_entry *f;
	while (!list_empty(&page_table))
	{
		if (pointer == list_end(&page_table)){
			pointer = list_begin(&page_table);
		}

		f = list_entry(pointer, struct frame_entry, elem);
		if (f->evict == 1)
			f->evict = 0;
		else 	
			break;
		
		pointer = list_next(pointer);
	}

	// struct list_elem *e = list_pop_front(&page_table);
	// struct frame_entry *f = list_entry(e, struct frame_entry, elem);


	//printf("start swap \n");
	swap_out(f);
	//printf("end swap \n");
	pointer = list_next(pointer);
	if (pointer == list_end(&page_table))
	{
		pointer = list_begin(&page_table);
		lock_acquire(&frame_lock);
		list_pop_back(&page_table);
		lock_release(&frame_lock);

	}
	else 
	{
		lock_acquire(&frame_lock);
		list_remove(list_prev(pointer));
		lock_release(&frame_lock);
	}

	pagedir_clear_page(f->thread->pagedir, f->page_number);
	uint8_t *frame = f->frame_number;
	free(f);
	return frame;
}

void
pointer_set (void)
{
	if (pointer == NULL)
		pointer = list_begin(&page_table);
}

void
frame_remove (struct spage_entry *spage_entry)
{
	struct frame_entry * f = (struct frame_entry *) spage_entry->pointer;
  
  	if (pointer == &f->elem)
  	{
  		pointer = list_next(pointer);
  		if (pointer == list_end(&page_table))
  			pointer = list_begin(&page_table);
  	}

  	lock_acquire(&frame_lock);
	list_remove(&f->elem);
	lock_release(&frame_lock);

	pagedir_clear_page(f->thread->pagedir, f->page_number);
	palloc_free_page((void *)(f->frame_number));
	free(f);
}
