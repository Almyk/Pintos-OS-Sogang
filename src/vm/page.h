#ifndef __VM__
#define __VM__

#include <hash.h>
#include "vm/frame.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "filesys/file.h"

struct spte * page_zero (void);
struct spte * page_file (struct file *, off_t , size_t , bool);
//struct spte * page_swap (struct swap_entry *);

enum { file, swap, zero }; // type of memory reference

struct spte
  {
    void *vaddr; // virtual address
    uint8_t type; // type of memory reference
    off_t offset; // file offset
    uint32_t read_bytes;
    uint32_t zero_bytes;
    bool writable; // is file writable?
    struct swap_entry *swap_elem;
  };

#endif
