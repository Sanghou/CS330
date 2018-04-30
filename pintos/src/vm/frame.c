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


static struct list frame_table;

void frame_init(){
	list_init(&frame_table);
}

bool allocate_frame_elem(unsigned pn, unsigned fn){
	struct frame_entry *fe;
	fe = malloc(sizeof(struct frame_entry));
	fe->thread = thread_current();
	fe->page_number = pn;
	fe->frame_number = fn;
	fe->evict = 0;
	list_push_back(&frame_table, &fe->elem);

}

bool deallocate_frame_elem(unsigned pn){
	struct frame_entry *f;
	struct list_elem *e;

	 for (e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)){
    	f= list_entry(e, struct frame_entry, elem);
    	if(f->page_number == pn){
    		list_remove(e);
    		free(f);
    		return true;
    	}
	}
	return false;
}