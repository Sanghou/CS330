#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
#include <lib/kernel/hash.h>


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