#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
#include "vm/frame.h"
#include "threads/palloc.h"
#include "devices/block.h"

static struct list *swap_table;

/*
   Creates a swap table list
   which manages swap slots.
*/
void 
swap_list_init (void){
	//swap_table = palloc_get_page();
	list_init(swap_table);
}

/*
   Writes frame information in swap disk.
   Manage the swap_table list
*/
void 
swap_in (){

}

/*
   Writes frame information in swap disk.
   Manage the swap_table list
   returns false when swap table is full.
*/
void
swap_out (struct frame_entry frame){
	enum block_type swap = BLOCK_SWAP;
	struct swap_entry *se;

	struct block *swap_slot = block_get_role(swap);
	//swap pointer malloc and the list malloc is left

	if (swap_slot == NULL){    //swap_slot is full
		PANIC();
	}

	se->page_number = frame->page_number; 		
	se->thread = frame->thread;

	void * paddr = (frame->frame_number) << 12;

	//write to a swap block.
	block_write(swap_slot, sizeof(*paddr), paddr);
	deallocate_frame_elem(frame->frame_number);
	
	// how to manage the supplementary page table?
}


