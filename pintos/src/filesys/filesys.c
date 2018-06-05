#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"

#ifdef FILESYS
#include "filesys/cache.h"
#endif


/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  struct dir *dir = dir_open_root();
  dir_add(dir, ".", 1);
  dir_add(dir, "..", 1);
  dir_close (dir);

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  cache_destroy(fs_device);
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
  block_sector_t inode_sector = 0;
#ifdef FILESYS
  struct dir *dir = dir_open_current ();
#else
  struct dir *dir = dir_open_root ();
#endif

  struct inode *inode = NULL;
  char * file_name = name;

  if (strrchr (name, '/') != NULL)
  {
    // inode = (struct inode *) find_dir (name);
    // if (inode == NULL)
    //   return false;
    // dir_close(dir);
    // dir = dir_open(inode);
    // char *tmp;
    // tmp = find_name (name);
    dir_close (dir);
    struct inode *inode = NULL;
    char pointer[sizeof(name)];
    char tmp[sizeof(name)];
    memcpy(pointer, name, sizeof(name));
    dir = dir_open_root();
  
    char *token, *saved_ptr;

    for (token = strtok_r (pointer, "/", &saved_ptr); token != NULL;
      token = strtok_r (NULL, "/", &saved_ptr))
    {
      memcpy(tmp, token, sizeof(token));
      if (dir_lookup(dir, token, &inode) && inode != NULL && !inode_is_file (inode))
      {
        dir_close(dir);
        dir_open(inode);
      }
    }
    file_name = tmp;
  }

  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, true)
                  && dir_add (dir, file_name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
#ifdef FILESYS
  struct dir *dir = dir_open_current ();
#else
  struct dir *dir = dir_open_root ();
#endif
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, name, &inode);
  dir_close (dir);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
#ifdef FILESYS
  struct dir *dir = dir_open_current ();
#else
  struct dir *dir = dir_open_root ();
#endif
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir); 

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
