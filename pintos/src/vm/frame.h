#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>

//frame table 
struct frame_entry
	{
		struct list_elem elem;
		struct thread * thread;
		unsigned page_number; //virtual page number
		unsigned frame_number; //frame table number
	 	unsigned evict;
	 	bool writable;
 	};

void frame_init (void);
struct frame_entry * allocate_frame_elem (uint8_t *upage, bool writable);
bool deallocate_frame_elem (unsigned pn);
uint8_t* evict (void);