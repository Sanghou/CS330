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
#ifdef VM
#include "vm/file_map.h"
#endif

static void syscall_handler (struct intr_frame *);


void set_child_info (struct child_info *info, tid_t child_pid, tid_t parent_pid);
bool is_valid_addr (void *addr);

//void* read (void **esp);

int read (struct intr_frame *f);
void terminate_error (void);
void terminate (void);

struct lock sys_lock;
struct semaphore load_exec_sync;
struct semaphore exec_sema;

int global_mmap_id = 0;

void
syscall_init (void) 
{
  	lock_init(&sys_lock);
    sema_init(&load_exec_sync, 0);
    sema_init(&exec_sema, 0);
  	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


static void
syscall_handler (struct intr_frame *f) 
{
  //read system call number 
  int sys_num = read(f);

  // printf("syscall_number : %d\n", sys_num);

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
      exec_sema_down();
  		int status = read(f);

  		tid_t child_pid = thread_current()->tid;

  		struct child_info *info = find_info(child_pid);

  		thread_current()->exit_status = status;

  		if (info == NULL)
      {
  			terminate();
   			break;
  		}

      info->exit_status = status;

  		info->is_waiting = false;

      // printf("SYS_EXIT end\n");
  		terminate();
  		break;
  	}




  	case SYS_EXEC:
    {
      // printf("SYS_EXEC START\n");
  		enum intr_level old_level;
  		old_level = intr_disable();

  		const char * cmd_line= (const char *) read(f);

  		if (!is_valid_addr(cmd_line)) 
      {
        f->eax = -1;
  			terminate_error();
  			break;
  		}
      exec_sema_down();

  		tid_t child_pid = process_execute (cmd_line);

      if (child_pid == TID_ERROR){
        f->eax = -1;
        break;
      }


  		f->eax = child_pid;
      exec_sema_up();
      
  		struct child_info *info = malloc(sizeof(struct child_info));
  		set_child_info(info, child_pid, thread_current()->tid);

  		intr_set_level(old_level);
      // printf("SYS_EXEC end \n");
  		break;
  	}




  	case SYS_WAIT:
    {
      // printf("SYS_WAIT start\n");
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
      sema_down(&info->sema);

  		f->eax = info->exit_status;
  		remove_child(&info->elem);
  		free(info);
      // printf("SYS_WAIT end\n");
  		break;
  	}




  	case SYS_CREATE:
  	{
      // printf("SYS_CREATE\n");
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
      // printf("SYS_REMOVE\n");
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
      // printf("SYS_OPEN\n");
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

      lock_release(&sys_lock);

      if (file == NULL){
        f->eax = -1;
        break;
      }

      int fd =  set_file_descript(file);

      f->eax = fd;

  		break;
  	}




  	case SYS_FILESIZE:
    {
      // printf("SYS_FILESIZE\n");
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
      // printf("SYS_READ\n");
      int fd = read(f);
      const char *buffer = (const char *) read(f);
      int size = read(f);
      int read_size = 0;
      int8_t tmp = 1;

      if (!is_valid_addr(buffer)) 
      {

        f->eax = -1;
        terminate_error();
        break;
      }

      struct file_descript *descript = find_file_descript(fd);
      if (fd != 0 && descript == NULL){
        f->eax = -1;
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
        // int page_per_buffer = size / PGSIZE;
        // // printf("page_per_buffer : %d\n",page_per_buffer);
        // int i;
        // for (i = 0; i< page_per_buffer; i++)
        // {
        //   int mini_buf = (size < PGSIZE) ? size: PGSIZE;
        //   read_size += file_read(descript->file, buffer, (off_t) mini_buf);
        //   size -= mini_buf;
        // }

        lock_acquire(&sys_lock);
        read_size += file_read(descript->file, buffer, (off_t) size);
        lock_release(&sys_lock);
      }
      f->eax = read_size;
      
  		break;
  	}
  	




  	case SYS_WRITE:
    { 
      // printf("SYS_WRITE\n");

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

      struct file_descript *descript = find_file_descript(fd);

      if (fd != 1 && descript == NULL){
        f->eax = 0;
        terminate_error();
        break;
      }

  		if (fd == 1)
      {
        lock_acquire(&sys_lock);
  			putbuf(buffer, size);
        lock_release(&sys_lock);
  			f->eax = size;
  		}
      else
      {
        lock_acquire(&sys_lock);
        write_size = file_write(descript->file, buffer, size);
        lock_release(&sys_lock);
        f->eax = write_size;
      }


  		break;
  	}





  	case SYS_SEEK:
    {
      // printf("SYS_SEEK\n");
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
      int fd = read(f);

      struct file_descript *descript = find_file_descript(fd);

      if (descript == NULL){
        break;
      }
      
      lock_acquire(&sys_lock);
      file_close(descript->file);
      lock_release(&sys_lock);
      
      remove_file(&descript->fd_elem); 
  		break;
  	}


    case SYS_MMAP:
    {

      unsigned int fd = read(f);
      void *addr =(void *)read(f);

      if(fd <= 1 || !is_user_vaddr(addr) || addr==0 ){
        f->eax = -1;
        break;
      }

      struct file_descript *descript = find_file_descript(fd);

      if(descript == NULL) {
        f->eax = -1;
        break;
      }

      struct file *file = file_reopen(descript->file);

      int file_len = file_length(file);

      if(file_len==0 ){
        f->eax = -1;
        break;
      }

      struct file_map* mapped_file = load_file(file, (uint8_t*)addr, (uint32_t) file_len, (ROUND_UP(file_len,PGSIZE) - file_len), true);
      if(mapped_file == NULL){
        f->eax = -1;
        break;
      }
      mapped_file->t = thread_current();
      mapped_file->file = file;
      mapped_file->mmap_id = global_mmap_id;
      global_mmap_id++;

      list_push_back(&thread_current()->mapping_table,&mapped_file->elem);

      f->eax = mapped_file->mmap_id;
      break;

    }

    case SYS_MUNMAP:
    {
      //list_init(&thread_current()->mapping_table);

      int mmap_id = read(f);

      struct list* mapping_table = &thread_current()->mapping_table;
      struct list_elem* e;
      for(e=list_begin(mapping_table); e!= list_end(mapping_table); e=list_next(e)){

        struct file_map * mapped_file = list_entry(e,struct file_map,elem);

        if(mapped_file->mmap_id == mmap_id){
          while(!list_empty(&mapped_file->addr)){
            struct list_elem* e2;
            e2 = list_pop_front(&mapped_file->addr);
            struct addr_elem *pointer = list_entry(e2,struct addr_elem, elem);
            
            //if((unsigned)pointer->physical_address&PTE_D){ //check dirty bit
            if(pagedir_is_dirty(thread_current()->pagedir, pointer->virtual_address)){ //check dirty bit
              acquire_sys_lock();
              file_write_at(mapped_file->file, pointer->physical_address, PGSIZE, pointer->ofs);
              release_sys_lock();
            }

            deallocate_spage_elem(pointer->virtual_address);
            deallocate_frame_elem(pointer->virtual_address);
            free(pointer);}
          }

        list_remove(&mapped_file->elem); 
        free(mapped_file);

        break;
    }
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
  sema_init(&info->sema,0);
	info->child_pid = child_pid;
  info->parent_pid = parent_pid;
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

  result = *(char **)f->esp;
  
  f->esp += 4;
  return result;
}


bool 
is_valid_addr(void *addr){
  
  return (is_user_vaddr(addr) && pagedir_get_page(thread_current()->pagedir, addr) != NULL);

}

void terminate()
{ 
  struct child_info *info = find_info(thread_current()->tid);
  sema_up(&thread_current()->start);
  if (info != NULL)
    sema_up(&info->sema);
  printf("%s: exit(%d)\n", thread_current()->name, thread_current()->exit_status);
  thread_exit();
}

void terminate_error(void)
{
  struct child_info *info = find_info(thread_current()->tid);
  if (info != NULL)
    sema_up(&info->sema);
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
load_exec_sync_up (void)
{
  sema_up(&load_exec_sync);
}
void
load_exec_sync_down (void)
{
  sema_down(&load_exec_sync);
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
