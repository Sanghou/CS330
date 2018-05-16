#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
//#include <stdio.h>
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
static struct list_elem *pointer;

void pointer_set (void);


void 
frame_init (void)
{
	list_init(&page_table);
	lock_init(&frame_lock);
	pointer = NULL;
}


struct frame_entry * allocate_frame_elem(uint8_t *upage){
	
	//printf("start allocate_frame_elem \n");
	uint8_t *kpage = palloc_get_page (PAL_USER);
	
	if(kpage == NULL){
		evict();
        kpage = palloc_get_page(PAL_USER);
        ASSERT(kpage != NULL);
	}
	
	struct frame_entry *fe;
	lock_acquire(&frame_lock);
	fe = malloc(sizeof(struct frame_entry));
	fe->thread = thread_current();
	fe->page_number = upage;
	fe->frame_number = kpage;
	fe->evict = 1;
	list_push_back(&page_table, &fe->elem);
	pointer_set();
	lock_release(&frame_lock);

	//printf("%u , %u \n", fe->page_number, fe->frame_number);
	return fe;
}


// struct frame_entry * allocate_frame_elem_both(uint8_t kpage, uint8_t upage){
	
// 	struct frame_entry *fe;
// 	lock_acquire(&frame_lock);
// 	fe = malloc(sizeof(struct frame_entry));
// 	fe->thread = thread_current();
// 	fe->page_number = upage;
// 	fe->frame_number = kpage;
// 	fe->evict = 0;
// 	list_push_back(&page_table, &fe->elem);
// 	lock_release(&frame_lock);

// 	return fe;
// }


/*
bool 
allocate_frame_elem(unsigned fn, unsigned pn)
{

	struct frame_entry *fe;
	fe = malloc(sizeof(struct frame_entry));
	fe->thread = thread_current();
	fe->page_number = pn;
	fe->frame_number = fn;
	fe->evict = 0;
	lock_acquire(&frame_lock);
	list_push_back(&page_table, &fe->elem);
	lock_release(&frame_lock);
}
*/
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
    		pagedir_clear_page(f->thread->pagedir, f->page_number);
    		palloc_free_page((void *)(f->frame_number));
    		free(f);
    		lock_release(&frame_lock);
    		return true;
    	  }
	  }
	lock_release(&frame_lock);
	return false;
}

void
evict (void) // FIFO;
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
	palloc_free_page((void *)(f->frame_number));
	free(f);
}

void
pointer_set (void)
{
	if (pointer == NULL)
		pointer = list_begin(&page_table);
}

