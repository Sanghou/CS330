#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>

struct swap_entry{
	struct list_elem list_elem; //swap_list elem;
	unsigned page_number; 		//page once assigned to this
	struct block *swap_block; 	//pointer to the data in disk.
	void *thread; 				//pointer to the thread which owned this virtual page.
	
};

void swap_list_init();
void swap_in();
void swap_out (struct frame_entry frame);
