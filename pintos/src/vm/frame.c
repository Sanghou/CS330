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

void
pointer_check (struct list_elem *list_elem)
{
	if (pointer == list_elem)
  	{
  		pointer = list_next(pointer);
  		if (pointer == list_end(&page_table))
  			pointer = list_begin(&page_table);
  	}
}


struct frame_entry * allocate_frame_elem(uint8_t *upage, bool writable, bool phys){
	
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
	if (phys){
		enum spage_type page_type = PHYS_MEMORY;
		allocate_spage_elem(fe->page_number, fe->frame_number, page_type, fe, writable);
	}
    
    lock_release(&frame_lock);
	return fe;
}


struct frame_entry * allocate_frame_elem_both(uint8_t upage, bool writable){
	

	//lock_acquire(&frame_lock);
	uint8_t *kpage = palloc_get_page (PAL_USER | PAL_ZERO);
	
	if(kpage == NULL){
		evict();
        kpage = palloc_get_page(PAL_USER | PAL_ZERO);
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

	enum spage_type page_type = PHYS_MEMORY;
	allocate_spage_elem(fe->page_number, fe->frame_number, page_type, fe, writable);

	lock_release(&frame_lock);
	return fe;
}

bool deallocate_frame_elem(struct thread *t, unsigned pn){
	struct frame_entry *f;
	enum spage_type type = SWAP_DISK;
	
	//lock_acquire(&frame_lock);

	struct spage_entry *spage_entry = mapped_entry(t, pn);
	if (spage_entry == NULL )
		return false;
	if (spage_entry->page_type == type)
		return false;
	f = (struct frame_entry *) spage_entry->pointer;
 	
 	
  	lock_acquire(&frame_lock);
  	pointer_check (&f->elem);
  	
	list_remove(&f->elem);

	hash_delete(&t->supplement_page_table, &spage_entry->elem);
	free(spage_entry);

	pagedir_clear_page(t->pagedir, f->page_number);
	palloc_free_page((void *)(f->frame_number));
	free(f);

	lock_release(&frame_lock);
	
	return true;
}

void
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

		else{
		// struct spage_entry * s = mapped_entry(f->thread,f->page_number);
		// printf("weqfqsdaf : %d \n", s->mmap);
				break;
		}

		pointer = list_next(pointer);	
	}

	swap_out(f);
	
	lock_acquire(&frame_lock);

	struct list_elem *e = pointer;
	pointer_check (pointer);
	list_remove(e);

	pagedir_clear_page(f->thread->pagedir, f->page_number);
	palloc_free_page((void *)(f->frame_number));
	free(f);

	lock_release(&frame_lock);
	
	// struct spage_entry *entry = mapped_entry(f->thread, f->page_number);
	// frame_remove(entry);
}

/*
	function necessary to remove only frame_entry using spage_entry
	used in thread_exit
*/
void
frame_remove (struct spage_entry *spage_entry)
{
	struct frame_entry *f = (struct frame_entry *) spage_entry->pointer;
  
  	lock_acquire(&frame_lock);
  	pointer_check (&f->elem);
  	
	list_remove(&f->elem);

	pagedir_clear_page(f->thread->pagedir, f->page_number);
	palloc_free_page((void *)(f->frame_number));
	free(f);

	lock_release(&frame_lock);
}

void
pointer_set (void)
{
	if (pointer == NULL)
		pointer = list_begin(&page_table);
}
void
acquire_frame_lock (void){
	lock_acquire(&frame_lock);
}
void
release_frame_lock (void)
{
	lock_release(&frame_lock);
}
