#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
static int get_user (const uint8_t *uaddr);
static bool put_user (uint8_t *udst, uint8_t byte);

int read(struct intr_frame *f);
// void write();
void terminate();

static struct lock sys_lock;

void
syscall_init (void) 
{
  lock_init(&sys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  // printf ("system call!\n");

  //to provide an  exclude operation for one user program.
  lock_acquire(&sys_lock);

  //read system call number 
  int sys_num = read(f);
  if (sys_num == -1) terminate();

  switch(sys_num){
  	/* System call for pintos proj2 */
  	case SYS_HALT:{
  		shutdown_power_off();
  		break;
  	}


  	case SYS_EXIT:{
  		int status = read(f);
  		if (status == -1) terminate();

  		//some code to transfer child end.

  		sema_up(&start);
  		// f->eax = status;
  		terminate();
  		break;
  	}

  	case SYS_EXEC:{

  		break;
  	}

  	case SYS_WAIT:{
  		sema_down(&start);



  		sema_up(&end);
  		break;
  	}


  	case SYS_CREATE:{
  		//read arguments

  		// f->eax = filesys_create(file, initial_size);
  		break;
  	}
  	case SYS_REMOVE:{
  		//read arguments

  		// f->eax = filesys_remove(file);
  		break;
  	}
  	case SYS_OPEN:{
  		//read arguments

  		// struct file *file = filesys_open(file_name);
  		break;
  	}
  	case SYS_FILESIZE:{


  		break;
  	}
  	case SYS_READ:{

  		break;
  	}
  	case SYS_WRITE:{
  		int fd = read(f);
  		if (fd == -1) terminate();
  		const void * buffer= *((const void **) f->esp); 
  		f->esp += 4;
  		unsigned size = (unsigned) read(f);
  		if (size == (unsigned) -1) terminate();

  		if (fd == 1){
  			putbuf(buffer, size);
  			f->eax = size;
  		}
  		break;
  	}
  	case SYS_SEEK:{
  		break;
  	}
  	case SYS_TELL:{
  		break;
  	}
  	case SYS_CLOSE:{
  		


  		break;
  	}
  	default:
  		terminate();
  		break;
  }

  lock_release(&sys_lock);
}

int read(struct intr_frame *f){

	void *addr = (void *)f->esp;
	
	if (!is_user_vaddr(addr)){
		return -1;
	}

	int result = get_user((const uint8_t *)addr);
	if (result == -1){
			return -1;
	}
	f->esp += 4; 

	return result;
}


void terminate(){
	lock_release(&sys_lock);
	thread_exit();
}

//code in the pintos document
/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int
get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}

struct lock *
get_sys_lock(void){
	return &sys_lock;
}
