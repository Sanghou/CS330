#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#ifdef VM
struct file_map *load_file (struct file *file, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable);
#endif

#endif /* userprog/process.h */
