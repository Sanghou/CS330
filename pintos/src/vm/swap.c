#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
#include "vm/swap.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "devices/block.h"

static struct list *swap_table;
static struct lock swap_lock;

/*
   Creates a swap table list
   which manages swap slots.
*/
void 
swap_list_init (void){
	swap_table = malloc(sizeof(struct list));
	list_init(swap_table);
	//\
	lock_init(&swap_lock);
}

/*
   Writes frame information in swap disk.
   Manage the swap_table list
*/
void 
swap_in (struct thread *t, unsigned page_num){
	//find the proper swap slot in the swap_table



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

	enum block_type swap = BLOCK_SWAP;
	struct swap_entry *se;

	struct block *swap_slot = block_get_role(swap);

	se = malloc(sizeof(struct swap_entry));

	if (swap_slot == NULL)
		{ 
			//swap_slot is full
			//PANIC();
		}

	se->page_number = frame->page_number; 		
	se->thread = frame->thread;
	se->swap_block = swap_slot;

	void * paddr = frame->frame_number;

	//write to a swap block.
	block_write(swap_slot, sizeof(*paddr), paddr);
	deallocate_frame_elem(frame->frame_number);

	list_push_back(swap_table, &se->list_elem);
	
	// how to manage the supplementary page table?
	//pagedir clear? 
}


