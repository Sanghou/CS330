#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
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
int sector;	//sector 대신에 pool 로 관리 

/*
   Creates a swap table list
   which manages swap slots.
*/
void 
swap_list_init (void){

	list_init(&swap_table);
	sector = 0;
	lock_init(&swap_lock);
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
	      	printf("FIND!!\n");
	      	struct frame_entry* fe = allocate_frame_elem(se->page_number);

	      	// allocate_spage_elem(t->page_number, t->frame_number);
	      	bool success = pagedir_set_page(t->pagedir, fe->page_number, fe->frame_number, true);
	      	if (!success) 
	      	{
	      		printf("no success\n");
	      		return false;
	      	}
	      	list_remove(e);	
	      	block_read (se->swap_slot, se->sector, (void *) fe->frame_number);
	      	free(se);
	        return true;
	      }
	  }
	return false;

	//block_read to the physical address pointer. 

	//delete the list element 
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

	se = malloc(sizeof(struct swap_entry));

	if (swap_slot == NULL)
		{ 
			//swap_slot is full
			PANIC("SWAP BLOCK IS FULL");
		}

	se->page_number = frame->page_number; 		
	se->thread = frame->thread;
	se->swap_slot = swap_slot;
	se->sector = sector;

	void * paddr = (frame->frame_number);

	//write to a swap block.
	block_write(swap_slot, sector, paddr);
	// deallocate_frame_elem(frame->frame_number);

	sector++;
	list_push_back(&swap_table, &se->list_elem);
	
	// how to manage the supplementary page table?
	
}