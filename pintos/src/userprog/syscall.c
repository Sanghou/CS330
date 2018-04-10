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
#include "threads/pte.h"
#include "userprog/pagedir.h"

static void syscall_handler (struct intr_frame *);





void set_child_info(struct child_info *info, tid_t child_pid, tid_t parent_pid);
bool is_valid_addr(void *addr);
int read (struct intr_frame *f, int pointer);
void terminate_error();
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
  int sys_num = read(f, 0);

  switch(sys_num)
  {
  	/* System call for pintos proj2 */
  	case SYS_HALT:
    {
  		shutdown_power_off();
  		break;
  	}

  	case SYS_EXIT:
    {
  		int status = read(f, 0);

  		tid_t child_pid = thread_current()->tid;

  		struct child_info *info = find_info(child_pid);

  		if (info == NULL)
      {
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

  	case SYS_EXEC:
    {
  		enum intr_level old_level;
  		//old_level = intr_disable();

  		const char * cmd_line= (const char *) read(f, 1);

  		if (!is_valid_addr(cmd_line)) 
      {
  			terminate_error();
  			break;
  		}
  		tid_t child_pid = process_execute (cmd_line);

  		f->eax = child_pid;

  		struct child_info *info = malloc(sizeof(struct child_info));
  		set_child_info(info, child_pid, thread_current()->tid);

  		//intr_set_level(old_level);

  		break;
  	}

  	case SYS_WAIT:
    {
      tid_t child_pid = (tid_t) read(f, 0);

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
  	case SYS_OPEN:
    {
  		//read arguments

  		// struct file *file = filesys_open(file_name);
  		// struct file_descript *file_ds = 

  		// set_file_descript( ,file);
  		break;
  	}
  	case SYS_FILESIZE:
    {


  		break;
  	}
  	case SYS_READ:
    {

  		break;
  	}
  	case SYS_WRITE:
    { 
  		int fd = read(f,0);
      const char *buffer = (const char *) read(f,1);
  		unsigned size = (unsigned) read(f, 0);

      if (!is_valid_addr(buffer)) 
      {
        terminate_error();
        break;
      }

  		if (fd == 1)
      {
  			putbuf(buffer, size);
  			f->eax = size;
  		}
  		break;
  	}
  	case SYS_SEEK:
    {
  		break;
  	}
  	case SYS_TELL:
    {
  		break;
  	}
  	case SYS_CLOSE:
    {
  		
  		break;
  	}
  	default:
  		terminate();
  		break;
  }

  lock_release(&sys_lock);
}

void set_child_info(struct child_info *info, tid_t child_pid, tid_t parent_pid)
{
	struct thread *child = get_thread_from_tid(child_pid);

	memset (info, 0, sizeof *info);
	info->child_pid = child_pid;
  	info->parent_pid = parent_pid;
  	info->sema = &child->start;
  	info->exit_status = -1;
  	info->is_waiting = false;

  	insert_child(&info->elem);
}

int 
read (struct intr_frame *f, int pointer)
{
  if (!is_valid_addr(f->esp)) {
    terminate_error();
    return -1;
  }
  int result;

  if (pointer)
  {
    result = *(char **)f->esp;
  }else
  {
    result = *(char *)f->esp;
  }
  
  f->esp += 4;
  return result;
}

bool 
is_valid_addr(void *addr)
{
  if (!is_user_vaddr(addr) || addr == NULL) return false;

  uint32_t *pd, *pde, *pt, *pte;
  uintptr_t tmp;
  asm volatile ("movl %%cr3, %0" : "=r" (tmp));
  pd = ptov (tmp);

  if (pd == NULL) return false;

  pde = pd + pd_no(addr);

  if (*pde == 0) return false;

  pt = pde_get_pt(*pde);

  if (pt == NULL) return false;

  pte = &pt[pt_no(addr)];

  if (pte == NULL) return false; //page table doesn't exist

  if (!(*pte&PTE_P)) return false; //page table entry not present

  return true;
}
void terminate()
{
  lock_release(&sys_lock);
  sema_up(&thread_current()->start);
  thread_exit();
}

void terminate_error()
{
	lock_release(&sys_lock);
  thread_current()->exit_status = -1;
  printf("%s: exit(%d)\n", thread_current()->name, -1);
  sema_up(&thread_current()->start);
	thread_exit();
}

struct lock *
get_sys_lock(void)
{
	return &sys_lock;
}
