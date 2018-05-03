#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
#include <lib/kernel/hash.h>

struct spage_entry
{
	struct hash_elem elem;
	struct thread * thread;
	unsigned va;
	unsigned pa;
};

void spage_init();
bool allocate_spage_elem(unsigned va, unsigned pa);
bool deallocate_spage_elem(unsigned va);
