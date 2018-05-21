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
#include "vm/file_map.h"

static struct list swap_table;
static struct lock swap_lock;
static struct bitmap *used_sector;


/*
   Creates a swap table list
   which manages swap slots.
*/

void 
swap_list_init (void){
	list_init(&swap_table);
	lock_init(&swap_lock);

	struct block * block = block_get_role(BLOCK_SWAP);
	block_sector_t sector_size = block_size(block);
	used_sector = bitmap_create(sector_size);
}

/*
   Writes frame information in swap disk.
   Manage the swap_table list
*/

void 
swap_in (struct spage_entry *spage_entry){

	struct block *swap_slot = block_get_role(BLOCK_SWAP);

	struct swap_entry *se = (struct swap_entry *) spage_entry->pointer;

	struct frame_entry* fe = allocate_frame_elem(se->page_number, spage_entry->writable, false);
	struct thread *t = thread_current();

    bool success = pagedir_set_page(t->pagedir, (void *) fe->page_number, (void *) fe->frame_number, spage_entry->writable);
    if (!success) 
    {
    	palloc_free_page((void *)fe->frame_number);
      	return;
    }
    lock_acquire(&swap_lock);
    list_remove(&se->list_elem);
    lock_release(&swap_lock);	

    int i;
    int sector_per_page = PGSIZE / BLOCK_SECTOR_SIZE;
    int sector = se->sector;

    for (i = 0 ; i < sector_per_page; i++)
	{
		void * tmp = malloc(BLOCK_SECTOR_SIZE);
		block_read (swap_slot, sector, tmp);
		memcpy(((void *) fe->frame_number+BLOCK_SECTOR_SIZE*i), tmp, BLOCK_SECTOR_SIZE);
  		free(tmp);
		sector++;
	}

	lock_acquire(&swap_lock);
	bitmap_set_multiple (used_sector, se->sector, sector_per_page, false);
	lock_release(&swap_lock);


	enum spage_type type = PHYS_MEMORY;
	spage_entry->pointer = fe;
	spage_entry->page_type = type;
	spage_entry->pa = fe->frame_number;
   
    free(se);
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

	enum spage_type type = SWAP_DISK;
	struct spage_entry * spage_entry= mapped_entry (frame->thread, frame->page_number);
	switch (spage_entry->page_type){
		case MMAP:
		{
			struct file_map * mapped_file = spage_entry->file_map;
			struct list_elem *e;
			for (e = list_begin(&mapped_file->addr); e != list_end(&mapped_file->addr); e = list_next(e)){
			    struct addr_elem *addr_elem = list_entry (e, struct addr_elem, elem);
			    if (addr_elem->va == spage_entry->va){
			      break;
			    }
			}
		  	struct addr_elem *addr_elem = list_entry (e, struct addr_elem, elem);

		  	if (pagedir_is_dirty(mapped_file->t->pagedir, spage_entry->va)){
		    	acquire_sys_lock();
		    	file_write_at(mapped_file->file, spage_entry->pa, PGSIZE, addr_elem->ofs);
		    	release_sys_lock();
	    	}
	    	spage_entry->pointer = NULL;
	    	free(se);
	    	return;
		}	
		case PHYS_MEMORY:
		{
			struct thread *t = frame->thread;
			spage_entry->page_type = type;
			spage_entry->pa = se->sector;
			spage_entry->pointer = se;
		}
	}
	

	//여기 block
	void * paddr = (void *) frame->frame_number;

	lock_acquire(&swap_lock);
	size_t sector = bitmap_scan_and_flip(used_sector, 0, sector_per_page, false);
	lock_release(&swap_lock);
	se->sector = sector; 

	if (sector == BITMAP_ERROR)
		PANIC("SWAP BLOCK IS FULL");

	//write to a swap block.
	for (i = 0 ; i < sector_per_page; i++)
	{
		paddr = ((void *) frame->frame_number) + BLOCK_SECTOR_SIZE * i;
		block_write(swap_slot, sector, paddr);
		sector++;
	}
	
	lock_acquire(&swap_lock);
	list_push_back(&swap_table, &se->list_elem);
	lock_release(&swap_lock);

	// how to manage the supplementary page table?
}

/*
	function necessary to remove only swap_entry using spage_entry
	used in thread_exit
*/
void
swap_remove (struct spage_entry *spage_entry)
{
	struct swap_entry *se = (struct swap_entry *) spage_entry->pointer;

    int sector_per_page = PGSIZE / BLOCK_SECTOR_SIZE;

    // lock_acquire(&swap_lock);
	list_remove(&se->list_elem);

	
	bitmap_set_multiple (used_sector, se->sector, sector_per_page, false);
	// lock_release(&swap_lock);

	free(se);

}
void
acquire_swap_lock (void){
	lock_acquire(&swap_lock);
}
void
release_swap_lock (void)
{
	lock_release(&swap_lock);
}