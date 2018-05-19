#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
#include <lib/kernel/hash.h>

struct file_map{
	struct thread *t;
	int fd;
	int mmap_id;
	struct list addr;
	struct list_elem elem;
}