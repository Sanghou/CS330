#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
#include <lib/kernel/bitmap.h>
#include "vm/swap.h"
#include "devices/block.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "threads/pte.h"
#include "userprog/pagedir.h"

static struct list swap_table;
static struct lock swap_lock;
static struct semaphore sema;
static struct bitmap *used_sector;


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
bool 
swap_in (struct thread *t, unsigned page_num){
	//find the proper swap slot in the swap_table
	struct list_elem *e;

	// struct block *swap_slot = block_get_role(BLOCK_SWAP);
	for (e = list_begin(&swap_table); e != list_end(&swap_table); e = list_next(e))
	  {
	    struct swap_entry *se = list_entry(e, struct swap_entry, list_elem);

	    if (t == se->thread && pg_no(se->page_number) == pg_no(page_num))
	      {
	      	// printf("FIND!!\n");
	      	struct frame_entry* fe = allocate_frame_elem(se->page_number);

	      	// allocate_spage_elem(t->page_number, t->frame_number);
	      	bool success = pagedir_set_page(t->pagedir, (void *) fe->page_number, (void *) fe->frame_number, true);
	      	if (!success) 
	      	{
	      		// printf("no success\n");
	      		return false;
	      	}

	      	list_remove(e);	

	      	int i;
			int sector_per_page = PGSIZE / 512;
			int sector = se->sector;

	      	for (i = 0 ; i < sector_per_page; i++)
			{
				void * tmp = malloc(512);
				
				block_read (se->swap_slot, sector, tmp);
				memcpy(((void *) fe->frame_number+512*i), tmp, 512);
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
	//swap block initialize in where?
	struct swap_entry *se;

	struct block *swap_slot = block_get_role(BLOCK_SWAP);

	int sector_number;
	int sector_per_page = PGSIZE / 512;

	int i;

	se = malloc(sizeof(struct swap_entry));

	if (swap_slot == NULL)
		{
			//swap_slot is full
			PANIC("CANNOT FIND SWAP DISK");
		}

	se->page_number = frame->page_number; 		
	se->thread = frame->thread;
	se->swap_slot = swap_slot;

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
		paddr = ((void *) frame->frame_number) + 512 * i;
		block_write(swap_slot, sector, paddr);
		sector++;
	}
	
	list_push_back(&swap_table, &se->list_elem);
	
	// how to manage the supplementary page table?
}

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

			int sector_per_page = PGSIZE / 512;
			int sector = se->sector;

			lock_acquire(&swap_lock);
			bitmap_set_multiple (used_sector, se->sector, sector_per_page, false);
			lock_release(&swap_lock);

	      	// free(se);
	      }
	  }
	
}