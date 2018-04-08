#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  printf ("system call!\n");

  //read system call number
  char *ptr = (char *)f->esp; 
  int sys_num = (int) *ptr;

  switch(sys_num){
  	/* System call for pintos proj2 */
  	case SYS_HALT:
  		break;
  	case SYS_EXIT:
  		// exit(0);
  		break;
  	case SYS_EXEC:
  		break;
  	case SYS_WAIT:
  		break;
  	case SYS_CREATE:
  		break;
  	case SYS_REMOVE:
  		break;
  	case SYS_OPEN:
  		break;
  	case SYS_FILESIZE:
  		break;
  	case SYS_READ:
  		break;
  	case SYS_WRITE:
  		printf("write");
  		break;
  	case SYS_SEEK:
  		break;
  	case SYS_TELL:
  		break;
  	case SYS_CLOSE:
  		break;
  	default:
  		break;
  }
  thread_exit ();
}
