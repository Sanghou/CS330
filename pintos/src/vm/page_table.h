#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
#include <lib/kernel/hash.h>
#include "threads/thread.h"
#include "filesys/off_t.h"


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

struct file_map{
	struct thread *t;
	struct file *file;
	int mmap_id;
	struct list addr;
	struct list_elem elem;
};

struct addr_elem{
	struct list_elem elem;
	off_t ofs;
	void * virtual_address;
	void * physical_address;
};

enum spage_type
	{
    /* Block device types that play a role in Pintos. */
    PHYS_MEMORY,
    SWAP_DISK,
    MMAP
	};

struct spage_entry
	{
		struct hash_elem elem;
		unsigned va; //virtual address
		unsigned pa;
		enum spage_type page_type;
		void * pointer;
	};

struct swap_entry
	{
		struct list_elem list_elem; //swap_list elem;
		unsigned page_number; 		//page once assigned to this
		void *thread; 				//pointer to the thread which owned this virtual page.
		int sector; 		//size of blocks.
		bool writable;
	};



void frame_init (void);
struct frame_entry * allocate_frame_elem (uint8_t *upage, bool writable);
bool deallocate_frame_elem (unsigned pn);
uint8_t* evict (void);


void spage_init (struct thread*);
bool allocate_spage_elem(unsigned va, enum spage_type flag);
bool deallocate_spage_elem(unsigned va);
struct spage_entry * mapped_entry (struct thread *t, unsigned va);
