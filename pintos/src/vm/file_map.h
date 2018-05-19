#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/kernel/list.h>
#include <lib/kernel/hash.h>
#include "filesys/off_t.h"

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
<<<<<<< HEAD
	void * virtual_address;
	void * physical_address;
};

=======
	void * spage_elem;
};
>>>>>>> cd1db56622332dcf7ea7e371d8300c4515869ed4
