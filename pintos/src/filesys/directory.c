#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "threads/thread.h"

/* A directory. */
struct dir 
  {
    struct inode *inode;                /* Backing store. */
    off_t pos;                          /* Current position. */
  };

/* A single directory entry. */
struct dir_entry 
  {
    block_sector_t inode_sector;        /* Sector number of header. */
    char name[NAME_MAX + 1];            /* Null terminated file name. */
    bool in_use;                        /* In use or free? */
  };

/* Creates a directory with space for ENTRY_CNT entries in the
   given SECTOR.  Returns true if successful, false on failure. */
bool
dir_create (block_sector_t sector, size_t entry_cnt)
{
  return inode_create (sector, entry_cnt * sizeof (struct dir_entry), false);
}

/* Opens and returns the directory for the given INODE, of which
   it takes ownership.  Returns a null pointer on failure. */
struct dir *
dir_open (struct inode *inode) 
{
  struct dir *dir = calloc (1, sizeof *dir);
  if (inode != NULL && dir != NULL)
    {
      dir->inode = inode;
      dir->pos = 0;
      return dir;
    }
  else
    {
      inode_close (inode);
      free (dir);
      return NULL; 
    }
}

/* Opens the root directory and returns a directory for it.
   Return true if successful, false on failure. */
struct dir *
dir_open_root (void)
{
  return dir_open (inode_open (ROOT_DIR_SECTOR));
}

struct dir *
dir_open_current (void)
{
  return dir_open (inode_open (thread_current()->DIR_SECTOR));
}

/* Opens and returns a new directory for the same inode as DIR.
   Returns a null pointer on failure. */
struct dir *
dir_reopen (struct dir *dir) 
{
  return dir_open (inode_reopen (dir->inode));
}

/* Destroys DIR and frees associated resources. */
void
dir_close (struct dir *dir) 
{
  if (dir != NULL)
    {
      inode_close (dir->inode);
      free (dir);
    }
}

/* Returns the inode encapsulated by DIR. */
struct inode *
dir_get_inode (struct dir *dir) 
{
  return dir->inode;
}

/* Searches DIR for a file with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool
lookup (const struct dir *dir, const char *name,
        struct dir_entry *ep, off_t *ofsp) 
{
  struct dir_entry e;
  size_t ofs;
  
  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (e.in_use && !strcmp (name, e.name)) 
      {
        if (ep != NULL)
          *ep = e;
        if (ofsp != NULL)
          *ofsp = ofs;
        return true;
      }
  return false;
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool
dir_lookup (const struct dir *dir, const char *name,
            struct inode **inode) 
{
  struct dir_entry e;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  if (lookup (dir, name, &e, NULL))
    *inode = inode_open (e.inode_sector);
  else
    *inode = NULL;

  return *inode != NULL;
}

/* Adds a file named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
bool
dir_add (struct dir *dir, const char *name, block_sector_t inode_sector)
{
  struct dir_entry e;
  off_t ofs;
  bool success = false;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);
  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;

  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL))
    goto done;

  if (BLOCK_SECTOR_SIZE - dir->pos < sizeof e)
    printf("DIRECTORY ADD\n");
    // allocate_sectors(1, &dir->inode->inode_disk);

  /* Set OFS to offset of free slot.
     If there are no free slots, then it will be set to the
     current end-of-file.
     
     inode_read_at() will only return a short read at end of file.
     Otherwise, we'd need to verify that we didn't get a short
     read due to something intermittent such as low memory. */
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (!e.in_use)
      break;

  /* Write slot. */
  e.in_use = true;
  strlcpy (e.name, name, sizeof e.name);
  e.inode_sector = inode_sector;
  success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

 done:
  return success;
}

/* Removes any entry for NAME in DIR.
   Returns true if successful, false on failure,
   which occurs only if there is no file with the given NAME. */
bool
dir_remove (struct dir *dir, const char *name) 
{
  struct dir_entry e;
  struct inode *inode = NULL;
  bool success = false;
  off_t ofs;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Find directory entry. */
  if (!lookup (dir, name, &e, &ofs))
    goto done;

  /* Open inode. */
  inode = inode_open (e.inode_sector);
  if (inode == NULL)
    goto done;

  /* Erase directory entry. */
  e.in_use = false;
  if (inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e) 
    goto done;

  /* Remove inode. */
  inode_remove (inode);
  success = true;

 done:
  inode_close (inode);
  return success;
}

/* Reads the next directory entry in DIR and stores the name in
   NAME.  Returns true if successful, false if the directory
   contains no more entries. */
bool
dir_readdir (struct dir *dir, char name[NAME_MAX + 1])
{
  struct dir_entry e;

  while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e) 
    {
      dir->pos += sizeof e;
      if (e.in_use)
        {
          strlcpy (name, e.name, NAME_MAX + 1);
          return true;
        } 
    }
  return false;
}

#ifdef FILESYS
bool
chdir (const char *name)
{
  struct inode *dir_inode;
  if (strrchr (name, '/') != NULL)
    dir_inode = (struct inode *) find_dir (name);
  else 
  {
    struct dir *dir = dir_open_root();
    if (!dir_lookup(dir, name, &dir_inode) || dir_inode == NULL || inode_is_file (dir_inode)){
      dir_close(dir);
      return false;
    }
    dir_close(dir); 
  }

  if (dir_inode == -1)
    return false;

  thread_current()->DIR_SECTOR = inode_get_inumber (dir_inode);
  return true; 
}

bool
mkdir (const char *name)
{
  struct dir *dir;
  struct inode * dir_inode = NULL;
  if (strrchr (name, '/') != NULL)
  {
    char pointer[sizeof(name)];
    char tmp[sizeof(name)];
    memcpy(pointer, name, sizeof(name));
    dir = dir_open_root();
    
    char *token, *saved_ptr;

    for (token = strtok_r (pointer, "/", &saved_ptr); token != NULL;
      token = strtok_r (NULL, "/", &saved_ptr))
    {
      memcpy(tmp, token, sizeof(token));

      if (dir_lookup(dir, token, &dir_inode) && dir_inode != NULL && !inode_is_file (dir_inode))
      {
        dir->inode = dir_inode;
      }
    }
    dir_inode = dir->inode;
    name = tmp;
  } 
  else 
    dir = dir_open_root();
  
  block_sector_t inode_sector;
  off_t ofs;
  struct dir_entry e;
  bool success = false;

  ASSERT (dir != NULL);

  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;

  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL))
    return false;

  if (BLOCK_SECTOR_SIZE - dir->pos < sizeof e)
    printf("DIRECTORY ADD\n");

  success = free_map_allocate (1, &inode_sector) & 
            inode_create (inode_sector, BLOCK_SECTOR_SIZE, false);

  /* Set OFS to offset of free slot.
     If there are no free slots, then it will be set to the
     current end-of-file.
     
     inode_read_at() will only return a short read at end of file.
     Otherwise, we'd need to verify that we didn't get a short
     read due to something intermittent such as low memory. */
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (!e.in_use)
      break;

  /* Write slot. */
  e.in_use = true;
  strlcpy (e.name, name, sizeof e.name);
  e.inode_sector = inode_sector;
  success = (inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e) & success;

  dir->inode = inode_open(inode_sector);
  dir_add(dir, ".", inode_sector); //. 추가하고 싶었음
  if (dir_inode == NULL)
    dir_add(dir, "..", 1); // .. 추가하고싶었음
  else{
    dir_add(dir, "..", inode_get_inumber(dir_inode));
    inode_close(dir_inode);
  }
  dir_close (dir);

  return success;

}
#endif
/*
  returns inode pointer of the last directory should be opened.
  void * 리턴하는 거 struct inode * 이고 싶었음.
*/
void *
find_dir (const char *name)
{
  struct inode *inode = NULL;
  char pointer[sizeof(name)];
  // char tmp[sizeof(name)];
  memcpy(pointer, name, sizeof(name));
  struct dir *dir = dir_open_root();
  
  char *token, *saved_ptr;

  for (token = strtok_r (pointer, "/", &saved_ptr); token != NULL;
    token = strtok_r (NULL, "/", &saved_ptr))
  {

    if (dir_lookup(dir, token, &inode) && inode != NULL && !inode_is_file (inode))
    {
      dir->inode = inode;
    }
    else 
      return -1;
  }
  dir_close(dir);
  inode = dir->inode;
  return inode;
}