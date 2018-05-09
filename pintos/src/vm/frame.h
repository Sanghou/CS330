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
};

void frame_init();
bool allocate_frame_elem(unsigned pn, unsigned fn);
bool deallocate_frame_elem(unsigned pn);
struct frame_entry * evict();