#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
#include "vm/frame.h"
#include "threads/thread.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"	


static struct list page_table;
static struct lock frame_lock;

// static unsigned elem_number;

void 
frame_init (void)
{
	list_init(&page_table);
	lock_init(&frame_lock);
	// elem_number = 0;
}

bool 
allocate_frame_elem(unsigned pn, unsigned fn)
{
	// if(elem_number == 1024)
	//   {
	// 	return NULL;
	//   }

	struct frame_entry *fe;
	fe = malloc(sizeof(struct frame_entry));
	fe->thread = thread_current();
	fe->page_number = pn;
	fe->frame_number = fn;
	fe->evict = 0;
	// elem_number++;
	lock_acquire(&frame_lock);
	list_push_back(&page_table, &fe->elem);
	lock_release(&frame_lock);
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
    		list_remove(e);
    		// elem_number--;
    		free(f);
    		lock_release(&frame_lock);
    		return true;
    	  }
	  }
	lock_release(&frame_lock);
	return false;
}

struct frame_entry * 
evict (void)
{
	struct list_elem *e = list_pop_front(&page_table);
	struct frame_entry *f = list_entry(e, struct frame_entry, elem);

	//swap_out
	// swap_out(f);

	//temporary

	lock_acquire(&frame_lock);
	list_remove(e);
	lock_release(&frame_lock);

	// palloc_free_page((void *)((f->frame_number)<< 12));
	palloc_free_page((void *)f->frame_number);
	free(f);
	return NULL;
}

// //find empty space function

// unsigned find_empty_frame(){
// 	if(elem_number == 1024){
// 		//evict();
// 	}
// 	else{

// 	}
// }