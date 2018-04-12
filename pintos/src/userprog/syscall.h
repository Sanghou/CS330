#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void release_sys_lock (void);
void acquire_sys_lock (void);
// void set_exec_success (bool success);


#endif /* userprog/syscall.h */
