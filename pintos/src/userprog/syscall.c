#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <list.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
static int get_user (const uint8_t *uaddr);
static bool put_user (uint8_t *udst, uint8_t byte);





void set_child_info(struct child_info *info, tid_t child_pid, tid_t parent_pid);
int read(struct intr_frame *f);
// void write();
void terminate();

static struct lock sys_lock;

void
syscall_init (void) 
{
	//list_init(&fd_list);
  	lock_init(&sys_lock);
  	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// void
// append_file (struct list_elem *elem){
// 	list_push_back(&fd_list);
// }

// void
// remove_file (struct list_elem *elem){
// 	list_remove(elem);
// }

// struct file_descript * 
// set_file_descript(struct file_descript *file_descript, struct file *file){

// 	struct thread *t = thread_current();

// 	int fd;

// 	memset (info, 0, sizeof *file_descript);

// 	file_descript->t = t;
//   	file_descript->file = file;

//   	file_descript->fd = 

//   	insert_child(&info->elem);
// }

// struct file_descript *
// find_file_descript(int fd, struct thead *t){
// 	struct list_elem *e;

// 	for(e=list_begin(&fd_list); e != list_end(&fd_list);e=list_next(e)){
// 		struct file_descript *file_descript = list_entry(e, struct file_descript, elem){
// 			if(file_descript->fd == fd && file_descript->t == t){
// 				return file_descript;
// 			}
// 		}
// 	}

// 	return NULL;
// }


static void
syscall_handler (struct intr_frame *f) 
{
  // printf ("system call!\n");

  //to provide an  exclude operation for one user program.
  lock_acquire(&sys_lock);

  //read system call number 
  if (read(f) == -1) terminate();
  int sys_num = *(char *)f->esp;
  f->esp += 4; 


  switch(sys_num){
  	/* System call for pintos proj2 */
  	case SYS_HALT:{
  		shutdown_power_off();
  		break;
  	}

  	case SYS_EXIT:{
  		if (read(f) == -1) terminate();
  		int status = *(char *)f->esp;
  		f->esp += 4; 

  		tid_t child_pid = thread_current()->tid;

  		struct child_info *info = find_info(child_pid);

  		if (info == NULL){
  			sema_up(&thread_current()->start);
  			printf("%s: exit(%d)\n", thread_current()->name,status);
  			terminate();
   			break;
  		}

  		info->exit_status = status;

  		sema_up(&thread_current()->start);

  		info->is_waiting = false;

  		terminate();
  		break;
  	}

  	case SYS_EXEC:{
  		enum intr_level old_level;
  		//old_level = intr_disable();

  		if (read(f) == -1) terminate();
  		const char * cmd_line= (const char *)*((const void **) f->esp);
  		// printf("cmd_line : %s, is same : %d\n",cmd_line, cmd_line=="child-simple");
  		f->esp += 4;

  		if (!is_user_vaddr(cmd_line)) {
  			terminate();
  			break;
  		}
  		tid_t child_pid = process_execute (cmd_line);

  		f->eax = child_pid;

  		struct child_info *info = malloc(sizeof(struct child_info));
  		set_child_info(info, child_pid, thread_current()->tid);

  		//intr_set_level(old_level);

  		break;
  	}

  	case SYS_WAIT:{
  		if (read(f) == -1) terminate();
  		tid_t child_pid =  *(char*)f->esp;
  		f->esp += 4; 

  		tid_t parent_pid = thread_current()->tid;

  		struct child_info *info = find_info(child_pid);

  		if (info == NULL || info->parent_pid != parent_pid || info->is_waiting )
  		{
  			//when given argument pid_t is not a child of current thread.
  			f->eax = -1;
  			break;
  		}

  		info->is_waiting = true;
  		sema_down(info->sema);

  		f->eax = info->exit_status;
  		remove_child(&info->elem);
  		free(info);

  		break;
  	}


  	case SYS_CREATE:
  	{
  		//read arguments

  		// f->eax = filesys_create(file, initial_size);
  		break;
  	}
  	case SYS_REMOVE:
  	{
  		//read arguments

  		// f->eax = filesys_remove(file);
  		break;
  	}
  	case SYS_OPEN:{
  		//read arguments

  		// struct file *file = filesys_open(file_name);
  		// struct file_descript *file_ds = 

  		// set_file_descript( ,file);
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
  		f->esp += 4; 
  		const void * buffer= *((const void **) f->esp); 
  		f->esp += 4;
  		unsigned size = (unsigned) read(f);
  		if (size == (unsigned) -1) terminate();
  		f->esp += 4; 

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

void set_child_info(struct child_info *info, tid_t child_pid, tid_t parent_pid){

	struct thread *child = get_thread_from_tid(child_pid);

	memset (info, 0, sizeof *info);
	info->child_pid = child_pid;
  	info->parent_pid = parent_pid;
  	info->sema = &child->start;
  	info->exit_status = -1;
  	info->is_waiting = false;

  	insert_child(&info->elem);
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
