#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/hash.h>

//frame table 
struct frame_entry
{
	struct hash_elem *hash_elem;
	int pa;
	int va;
	int evit;
};

void frame_init();
bool allocate_frame_elem(unsigned pa, unsigned va);
bool deallocate_frame_elem(unsigned pa);
struct frame_entry * evit();