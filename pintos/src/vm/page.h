#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
#include <lib/kernel/hash.h>
#include "threads/thread.h"

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
		enum spage_type page_type;
		void * pointer;
	};

void spage_init (struct hash *page_table);
bool allocate_spage_elem(unsigned va, enum spage_type flag, void * entry);
bool deallocate_spage_elem(unsigned va);
struct spage_entry * mapped_entry (struct thread *t, unsigned va);
void destroy_spage (struct hash *page_table);
