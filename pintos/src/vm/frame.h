#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
#include "vm/page.h"

//frame table 
struct frame_entry
	{
		struct list_elem elem;
		struct thread * thread;
		unsigned page_number; //virtual page number
		unsigned frame_number; //frame table number
	 	unsigned evict;
 	};

void frame_init (void);
// struct frame_entry * allocate_frame_elem (uint8_t *upage);
struct frame_entry * allocate_frame_elem(uint8_t *upage, bool writable, bool phys);
bool deallocate_frame_elem (struct thread *t, unsigned pn);
void frame_remove (struct spage_entry *spage_entry);
void evict (void);