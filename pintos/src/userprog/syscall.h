#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
struct lock *get_sys_lock(void);

#endif /* userprog/syscall.h */
