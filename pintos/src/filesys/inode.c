#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#ifdef FILESYS
#include "filesys/cache.h"
#endif

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
// #define DIRECT = 120;
// #define INDIRECT = 128;
// #define DOUBLE_INDIRECT = 128*128;



/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t direct[120];               /* First data sector. */
    block_sector_t indirect;                  /* Indirect index block */
    block_sector_t double_indirect;           /* Double indirect index block */
    uint32_t direct_number;   
    uint32_t indirect_number;
    uint32_t double_indirect_number;
    off_t length;                             /* File size in bytes. */
    unsigned magic;                           /* Magic number. */
    uint32_t is_file;                         /* Not used. */
  };

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
    bool grow;                          /* Grow */
  };

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos) 
{
  ASSERT (inode != NULL);
  int current_sector = inode->data.direct_number + inode->data.indirect_number + inode->data.double_indirect_number;
  if (pos > current_sector*BLOCK_SECTOR_SIZE || pos < 0)
    return -1;

  int result = pos / BLOCK_SECTOR_SIZE;

  if (result < 120)
    return inode->data.direct[result];

  block_sector_t buffer[128];

  if (result < 248)  /* indirect */
  {
    ASSERT (inode->data.indirect != -1);

    block_read(fs_device, inode->data.indirect, buffer);
    result -= 120;
    result = buffer[result];
  }
  else  /* double_indirect */
  {
    ASSERT (inode->data.double_indirect != -1);
    result -= 248;

    block_sector_t buffer2[128];

    block_read(fs_device, inode->data.double_indirect, buffer);
    int index = buffer[result / 128];
    block_read(fs_device, index, buffer2);
    result = buffer2[result % 128];
  }

  return result;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocatelocation fails. */

bool
allocate_sectors(size_t sectors, struct inode_disk* disk_inode){

  int i;
  bool success;
  
  block_sector_t location; 

  static char zeros[BLOCK_SECTOR_SIZE];

  if (sectors == 0) return true;

  for(i=0; i < sectors; i++)
  {
    success = free_map_allocate(1, &location);
    next_append(location, disk_inode);
    block_write(fs_device, location, zeros);
  }

  return success;
}

void
next_append(block_sector_t location, struct inode_disk* disk_inode){
  // check direct_number
  // if allocated sectors are less than 120, then allocate direct_sectors.

  if(disk_inode->direct_number < 120)
  {
    disk_inode->direct[disk_inode->direct_number] = location;
    disk_inode->direct_number++;
  }

  else
  {
    //direct == 120
    //Allocate indirect block.

    block_sector_t b[128];

    if(disk_inode->indirect_number == 0)
    {
      free_map_allocate(1, &disk_inode->indirect);
      block_write(fs_device, disk_inode->indirect , b);
    }

    if(disk_inode->indirect_number < 128)
    {
      block_read(fs_device, disk_inode->indirect, b);
      b[disk_inode->indirect_number] = location;
      block_write(fs_device, disk_inode->indirect,b);
      disk_inode->indirect_number++;
    }

    //indirect == 128, need to do double indirect
    
    // ----------- -------- ----------------
    // |         | |      | |              |
    // | first   | |second| |   data block |
    // |         | |      | |              |
    // ----------- -------- -----------------

    else
    {

    block_sector_t first[128];
    block_sector_t second[128];

    //first allocate block.
    //need first_indirect_block.

      if(disk_inode->double_indirect_number == 0)
      {
        block_sector_t first_double;
        free_map_allocate(1, &disk_inode->double_indirect);
        free_map_allocate(1, &first_double);
        first[0] = first_double;
        block_write(fs_device, disk_inode->double_indirect, first);
      }

      //read first indirect_block.
      ASSERT(disk_inode->double_indirect != -1);

      block_read(fs_device,disk_inode->double_indirect, first);

      uint32_t first_check = disk_inode->double_indirect_number / 128;
      uint32_t second_check = disk_inode->double_indirect_number % 128;

      if (second_check == 0 && first_check != 0)
      {
        block_sector_t first_double;
        free_map_allocate(1, &first_double);
        first[first_check] = first_double;
        block_write(fs_device, disk_inode->double_indirect, first); 
      }

      block_read(fs_device, first[first_check],second);
      second[second_check] = location;
      block_write(fs_device, first[first_check] ,second);

      disk_inode->double_indirect_number++;
    }
  }

}

bool
inode_create (block_sector_t sector, off_t length, bool is_file)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length); // return the needed sector number
      //from here i change code.
      disk_inode->magic = INODE_MAGIC;
      disk_inode->length = length;
      disk_inode->direct[0] = -1;
      disk_inode->indirect = -1;
      disk_inode->double_indirect = -1;
      disk_inode->direct_number = 0;
      disk_inode->indirect_number = 0;
      disk_inode->double_indirect_number = 0;
      disk_inode->is_file = is_file;

      success = allocate_sectors(sectors, disk_inode);

      block_write (fs_device, sector, disk_inode);

      free(disk_inode);
    }
  return success;
}

/* Allocate n sectors to not cnt */




/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  inode->grow = false;
  block_read (fs_device, inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

bool
inode_is_file (const struct inode *inode)
{
  return (bool) inode->data.is_file;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
      if (inode->grow)
        block_write(fs_device, inode->sector, &inode->data);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          free_map_release (inode->sector, 1);
          deallocate_sectors(inode);
          // free_map_release (inode->data.direct[0],
          //                   bytes_to_sectors (inode->data.length)); 
        }

      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
        #ifdef FILESYS
          cache_read (fs_device, sector_idx, buffer + bytes_read);
        #else
          block_read (fs_device, sector_idx, buffer + bytes_read);
        #endif
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                  break;
            }
        #ifdef FILESYS
          cache_read (fs_device, sector_idx, bounce);
        #else
          block_read (fs_device, sector_idx, bounce);
        #endif
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
    return 0;

  if (offset + size > ROUND_UP(inode_length(inode), BLOCK_SECTOR_SIZE))
  {
    //grow file!
    int file = offset+size;
    file -= ROUND_UP(inode_length(inode), BLOCK_SECTOR_SIZE);
    if (!allocate_sectors(DIV_ROUND_UP(file, BLOCK_SECTOR_SIZE), &inode->data)) 
      PANIC("did not allocate sectors\n");
    inode->data.length = offset+size;
    inode->grow = true;
  }
  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = ROUND_UP(inode_length (inode), BLOCK_SECTOR_SIZE) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
        #ifdef FILESYS
          cache_write (fs_device, sector_idx, buffer + bytes_written);
        #else
          block_write (fs_device, sector_idx, buffer + bytes_written);
        #endif
        }
      else 
        {
          /* We need a bounce buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left) 
          {
          #ifdef FILESYS
            cache_read (fs_device, sector_idx, bounce);
          #else
            block_read (fs_device, sector_idx, bounce);
          #endif
          } 
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
        #ifdef FILESYS
          cache_write (fs_device, sector_idx, bounce);
        #else
          block_write (fs_device, sector_idx, bounce);
        #endif
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);
  if (inode_length(inode) < offset)
    inode->data.length = offset;

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

void 
deallocate_sectors (const struct inode *inode)
{
  int i;
  for (i = 0; i < inode->data.direct_number; i++)
    free_map_release (inode->data.direct[i], 1);

  if (inode->data.indirect_number > 0)
  {
    ASSERT (inode->data.indirect != -1);

    block_sector_t buffer[128];
    block_read(fs_device, inode->data.indirect, buffer);

    int cnt = inode->data.indirect_number;

    while (cnt > 0)
    {
      cnt--;
      free_map_release (buffer[cnt], 1);   
    }

    free_map_release (inode->data.indirect , 1);
  }

  if (inode->data.double_indirect_number > 0)
  {
    ASSERT (inode->data.double_indirect != -1);

    block_sector_t buffer[128];
    block_read(fs_device, inode->data.double_indirect, buffer);
    i = inode->data.double_indirect_number;
    i = i / BLOCK_SECTOR_SIZE + 1;
    int cnt = inode->data.double_indirect_number;

    while (i > 0)
    {
      i--;
      int index = buffer[i];
      int j;
      block_sector_t buffer2[128];
      
      block_read(fs_device, index, buffer2);

      for (j = 0; j < 128; j++)
      {
        if (cnt <= 0)
          break;
        free_map_release(buffer2[j], 1);
        cnt--;
      }

      free_map_release(index, 1);
    }

    free_map_release (inode->data.double_indirect , 1);
  }

}