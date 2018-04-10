#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

// typedef int tid_t;

void syscall_init (void);
struct lock *get_sys_lock(void);

#endif /* userprog/syscall.h */
