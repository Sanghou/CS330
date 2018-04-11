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


void set_child_info (struct child_info *info, tid_t child_pid, tid_t parent_pid);
bool is_valid_addr (void *addr);
int read (struct intr_frame *f, int pointer);
void terminate_error (void);
void terminate (void);

static struct lock sys_lock;

void
syscall_init (void) 
{
	//list_init(&fd_list);
  	lock_init(&sys_lock);
  	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


static void
syscall_handler (struct intr_frame *f) 
{
  // printf ("system call!\n");

  //to provide an  exclude operation for one user program.
  //lock_acquire(&sys_lock);

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

  		thread_current()->exit_status = status;

  		if (info == NULL)
      {
  			// sema_up(&thread_current()->start);
  			// printf("%s: exit(%d)\n", thread_current()->name,status);
  			terminate();
   			break;
  		}

  		info->exit_status = status;

  		// sema_up(&thread_current()->start);

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
        f->eax = -1;
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
      const char *file = (const char *) read(f,1);
      unsigned size = (unsigned) read(f,0);

      if (!is_valid_addr(file)) 
      {
        terminate_error();
        break;
      }
      lock_acquire(&sys_lock);

  		f->eax = filesys_create(file, size);

      lock_release(&sys_lock);
  		break;
  	}

  	case SYS_REMOVE:
  	{
  		//read arguments
      const char *file = (const char *) read(f,1);

      if (!is_valid_addr(file)) 
      {
        terminate_error();
        break;
      }
      lock_acquire(&sys_lock);

  		f->eax = filesys_remove(file);
      lock_release(&sys_lock);
  		break;
  	}

  	case SYS_OPEN:
    {
  		//read arguments
      const char *file_name = (const char *) read(f,1);
      char *tmp = "";

      if (!is_valid_addr(file_name)) 
      {
        f->eax = -1;
        terminate_error();
        break;
      }

      if (!strcmp(file_name,tmp)){
      	f->eax =  -1;
      	terminate();
      	break;
      }
      lock_acquire(&sys_lock);


      struct file *file = filesys_open(file_name);
      int fd =  set_file_descript(file);

      f->eax = fd;

      lock_release(&sys_lock);
  		break;
  	}


  	case SYS_FILESIZE:
    {
      int fd = read(f, 0);
      
      struct file_descript *descript = find_file_descript(fd);

      if (descript == NULL){
        terminate_error();
        break;
      }

      lock_acquire(&sys_lock);

      f->eax = (int) file_length(descript->file);

      lock_release(&sys_lock);
  		break;
  	}
  	


  	case SYS_READ:
    {
      int fd = read(f,0);
      const char *buffer = (const char *) read(f,1);
      unsigned size = (unsigned) read(f, 0);
      int read_size = 0; 

      if (!is_valid_addr(buffer)) 
      {
        terminate_error();
        break;
      }

      struct file_descript *descript = find_file_descript(fd);

      if (fd != 0 && descript == NULL){
        terminate_error();
        break;
      }

      lock_acquire(&sys_lock);

      if (fd == 0)
      {
        //keyboard input
      } else {
        read_size = (int) file_read(descript->file, buffer, size);
      }
      f->eax = read_size;
      lock_release(&sys_lock);
  		break;
  	}
  	

  	case SYS_WRITE:
    { 
      int fd = read(f,0);
      const char *buffer = (const char *) read(f,1);
  		unsigned size = (unsigned) read(f, 0);
      int write_size = 0;

      if (!is_valid_addr(buffer)) 
      {
        terminate_error();
        break;
      }

      struct file_descript *descript = find_file_descript(fd);

      if (fd != 1 && descript == NULL){
        terminate_error();
        break;
      }

      lock_acquire(&sys_lock);

  		if (fd == 1)
      {
  			putbuf(buffer, size);
  			f->eax = size;
  		}
      else
      {
        write_size = (int) file_write(descript->file, buffer, size);
      }

      lock_release(&sys_lock);
  		break;
  	}
  	case SYS_SEEK:
    {
      int fd = read(f, 0);
      unsigned position = (unsigned) read(f, 0);

      struct file_descript *descript = find_file_descript(fd);

      if (descript == NULL){
        terminate_error();
        break;
      }

      lock_acquire(&sys_lock);

      file_seek(descript->file, position);

      lock_release(&sys_lock);

  		break;
  	}
  	case SYS_TELL:
    {
      int fd = read(f, 0);

      struct file_descript *descript = find_file_descript(fd);

      if (descript == NULL){
        terminate_error();
        break;
      }

      lock_acquire(&sys_lock);

      f->eax = (unsigned) file_tell(descript->file);

      lock_release(&sys_lock);
  		break;
  	}
  	case SYS_CLOSE:
    {
      int fd = read(f, 0);

      struct file_descript *descript = find_file_descript(fd);

      if (descript == NULL){
        terminate_error();
        break;
      }
      
      lock_acquire(&sys_lock);

      remove_file(&descript->fd_elem);
      file_close(descript->file);

      lock_release(&sys_lock);

  		break;
  	}
  	default:
  		terminate();
  		break;
  }

}

void 
set_child_info(struct child_info *info, tid_t child_pid, tid_t parent_pid)
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
  sema_up(&thread_current()->start);
  printf("%s: exit(%d)\n", thread_current()->name, thread_current()->exit_status);
  thread_exit();
}

void terminate_error()
{
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
