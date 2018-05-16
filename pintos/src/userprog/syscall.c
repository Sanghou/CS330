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

//void* read (void **esp);

int read (struct intr_frame *f);
void terminate_error (void);
void terminate (void);

struct lock sys_lock;
struct semaphore exec_sema;

void
syscall_init (void) 
{
  	lock_init(&sys_lock);
    sema_init(&exec_sema, 0);
  	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


static void
syscall_handler (struct intr_frame *f) 
{
/*
  if(!is_valid_addr(f->esp) || !is_valid_addr(*f->eip)){
    terminate_error();
  }
*/
  //read system call number 
  int sys_num = read(f);

  switch(sys_num)
  {
  	/* System call for pintos proj2 */

    if(!is_valid_addr(f->esp)){
      terminate_error();
    }



  	case SYS_HALT:
    {
  		shutdown_power_off();
  		break;
  	}



  	case SYS_EXIT:
    {
  		//int status = read(esp+1);
  		int status = read(f);

  		tid_t child_pid = thread_current()->tid;

  		struct child_info *info = find_info(child_pid);

  		thread_current()->exit_status = status;

  		if (info == NULL)
      {
  			// sema_up(&thread_current()->start);
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
  		old_level = intr_disable();

//  		const char * cmd_line= (const char *) read(esp+1);
  		const char * cmd_line= (const char *) read(f);

  		if (!is_valid_addr(cmd_line)) 
      {
        f->eax = -1;
  			terminate_error();
  			break;
  		}

  		tid_t child_pid = process_execute (cmd_line);

      if (child_pid == TID_ERROR){
        f->eax = -1;
        break;
      }

  		f->eax = child_pid;
  		struct child_info *info = malloc(sizeof(struct child_info));
  		set_child_info(info, child_pid, thread_current()->tid);
  		intr_set_level(old_level);

  		break;
  	}




  	case SYS_WAIT:
    {
     // tid_t child_pid = (tid_t) read(esp+1);
      tid_t child_pid = (tid_t) read(f);

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
     // const char *file = (const char *) read(esp+1);
      
      //int size = read(esp+2);
      const char *file = (const char *) read(f);
      int size = read(f);

      if (!is_valid_addr(file)) 
      {
        f->eax = 0;
        terminate_error();
        break;
      }
      lock_acquire(&sys_lock);

  		f->eax = filesys_create(file, (size_t)size);

      lock_release(&sys_lock);
  		break;
  	}




  	case SYS_REMOVE:
  	{
  		//read arguments
//      const char *file = (const char *) read(esp+1);
      const char *file = (const char *) read(f);

      if (!is_valid_addr(file)) 
      {
        f->eax = 0;
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
      //const char *file_name = (const char *) read(esp+1);
      const char *file_name = (const char *) read(f);
      char *tmp = "";

      if (!is_valid_addr(file_name)) 
      {
        f->eax = -1;
        terminate_error();
        break;
      }

      if (!strcmp(file_name,tmp)){
      	f->eax =  -1;
      	break;
      }
      lock_acquire(&sys_lock);

      struct file *file = filesys_open(file_name);

      if (file == NULL){
        f->eax = -1;
        lock_release(&sys_lock);
        break;
      }

      int fd =  set_file_descript(file);

      f->eax = fd;

      lock_release(&sys_lock);
  		break;
  	}






  	case SYS_FILESIZE:
    {
      //int fd = read(esp+1);
      int fd = read(f);
      
      struct file_descript *descript = find_file_descript(fd);

      if (descript == NULL){
        f->eax = 0;
        break;
      }

      lock_acquire(&sys_lock);

      f->eax = file_length(descript->file);

      lock_release(&sys_lock);
  		break;
  	}
  	




  	case SYS_READ:
    {

//      int fd = read(esp+1);
      
  //    const char *buffer = (const char *) read(esp+2);

    //  int size = read(esp+3);

      int fd = read(f);
      const char *buffer = (const char *) read(f);
      int size = read(f);
      int read_size = 0;
      int8_t tmp = 1;

      //printf("%p \n", buffer);

      if (!is_valid_addr(buffer)) 
      {

        f->eax = -1;
        terminate_error();
        break;
      }
      lock_acquire(&sys_lock);

      struct file_descript *descript = find_file_descript(fd);
      if (fd != 0 && descript == NULL){
        f->eax = -1;
        lock_release(&sys_lock);
        terminate_error();
        break;
      }

      if (fd == 0)
      {
        while (tmp != '\0'){
          tmp = input_getc();
          memcpy(buffer, tmp, 1);
          buffer++;
          read_size++;
          //putbuf();
        }

      } else {

        read_size = file_read(descript->file, buffer, (off_t) size);
      }
      f->eax = read_size;
      lock_release(&sys_lock);
      
  		break;
  	}
  	




  	case SYS_WRITE:
    { 

//      int fd = read(esp+1);
  //    const char *buffer = (const char *) read(esp+2);
  //		int size = read(esp+3);
      int fd = read(f);
      const char *buffer = (const char *) read(f);
  		int size = read(f);

      int write_size = 0;

      if (!is_valid_addr(buffer)) 
      {
        f->eax = 0;
        terminate_error();
        break;
      }

      lock_acquire(&sys_lock);

      struct file_descript *descript = find_file_descript(fd);

      if (fd != 1 && descript == NULL){
        f->eax = 0;
        lock_release(&sys_lock);
        terminate_error();
        break;
      }

  		if (fd == 1)
      {
  			putbuf(buffer, size);
  			f->eax = size;
  		}
      else
      {
        write_size = file_write(descript->file, buffer, size);
        f->eax = write_size;
      }

      lock_release(&sys_lock);

  		break;
  	}





  	case SYS_SEEK:
    {
      //int fd = read(esp+1);
      //int position = read(esp+2);
      int fd = read(f);
      int position = read(f);

      struct file_descript *descript = find_file_descript(fd);

      if (descript == NULL){
        break;
      }

      lock_acquire(&sys_lock);

      file_seek(descript->file, position);

      lock_release(&sys_lock);

  		break;
  	}
  	case SYS_TELL:
    {
      //int fd = read(esp+1);
      int fd = read(f);

      struct file_descript *descript = find_file_descript(fd);

      if (descript == NULL){
        f->eax = -1;
        break;
      }

      lock_acquire(&sys_lock);

      f->eax = file_tell(descript->file);

      lock_release(&sys_lock);
  		break;
  	}





  	case SYS_CLOSE:
    {
      //int fd = read(esp+1);
      int fd = read(f);

      struct file_descript *descript = find_file_descript(fd);

      if (descript == NULL){
        break;
      }
      
      lock_acquire(&sys_lock);

      
//      file_close(descript->file);
//        remove_file(&descript->fd_elem);
      file_close(descript->file);
      remove_file(&descript->fd_elem); 

      lock_release(&sys_lock);

  		break;
  	}
  	default:
  		terminate();
  		break;
  }

  // printf("%p\n", f->eip);
  // if (is_user_vaddr((char *)f->eip)){
  //   terminate_error();
  // }
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
read (struct intr_frame *f)
{
  if (!is_valid_addr(f->esp)) {
    terminate_error();
    return -1;
  }

  int result;
/*
  if (pointer)
  {
    result = *(char **)f->esp;

  }else

  {
    result = *(char *)f->esp;
  }
*/
  result = *(char **)f->esp;
  
  f->esp += 4;
  return result;
}
/*
void*
read (void **esp){
  if(!is_valid_addr(esp)){
    terminate_error();
    return -1;
  }
  return *esp;
}
*/

/*
bool 
is_valid_addr(void *addr)
{
  if (!is_user_vaddr(addr) || addr == NULL) return false;

  //printf("%d \n", *(unsigned *)addr);

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

  //if (!(*pte&PTE_U)) return false;

  return true;
}
*/

bool 
is_valid_addr(void *addr){
  
  return (is_user_vaddr(addr) && pagedir_get_page(thread_current()->pagedir, addr) != NULL);
}

void terminate()
{ 
  sema_up(&thread_current()->start);
  printf("%s: exit(%d)\n", thread_current()->name, thread_current()->exit_status);
  thread_exit();
}

void terminate_error(void)
{
  thread_current()->exit_status = -1;
  printf("%s: exit(%d)\n", thread_current()->name, -1);
  sema_up(&thread_current()->start);
	thread_exit();
}

void
release_sys_lock(void)
{
	lock_release(&sys_lock);
}

void 
acquire_sys_lock (void)
{
  lock_acquire(&sys_lock);
}

void
exec_sema_up (void)
{
  sema_up(&exec_sema);
}
void
exec_sema_down (void)
{
  sema_down(&exec_sema);
}
