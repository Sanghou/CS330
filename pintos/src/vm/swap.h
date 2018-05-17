#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
#include "threads/thread.h"
#include "vm/frame.h"

struct swap_entry
	{
		struct list_elem list_elem; //swap_list elem;
		unsigned page_number; 		//page once assigned to this
		void *thread; 				//pointer to the thread which owned this virtual page.
		int sector; 		//size of blocks. 
	};

void swap_list_init ();
bool swap_in (struct thread *t, unsigned page_num);
void swap_out (struct frame_entry *frame);
void swap_remove (struct thread *t);