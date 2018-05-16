#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
#include <lib/kernel/hash.h>

struct spage_entry
	{
		struct hash_elem elem;
		struct thread * thread;
		unsigned va; //virtual address
		unsigned pa; //physical address
		unsigned evict;
	};

enum spage_type
	{
    /* Block device types that play a role in Pintos. */
    PHYS_MEMORY,
    SWAP_DISK
	};

void spage_init (void);
bool allocate_spage_elem(unsigned va, unsigned pa);
bool deallocate_spage_elem(unsigned va);
struct spage_entry * mapped_entry (struct thread *t, unsigned va);