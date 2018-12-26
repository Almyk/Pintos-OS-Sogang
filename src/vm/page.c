#include "vm/page.h"

#include "threads/malloc.h"

struct spte *
create_zero_page (void)
{
  struct spte *p = (struct spte *) malloc (sizeof (struct spte));
  p->type = zero;
  p->offset = 0;
  p->read_bytes = 0;
  p->zero_bytes = (1 << 12); // everything is zero_bytes
  return p;
}

struct spte *
create_file_page (struct file *src, off_t offset, size_t zero_bytes, bool writable)
{
  struct spte *p = (struct spte *) malloc (sizeof (struct spte));
  p->type = file;
  p->file = src;
  p->offset = offset;
  p->zero_bytes = zero_bytes;
  p->writable = writable;
  return p;
}

/*
struct spte *
page_swap (struct swap_entry *swap_loc)
{
  struct spte *p = (struct spte *) malloc (sizeof (struct spte));
  p->type = swap;
  p->offset = 0;
  p->read_bytes = 0;
  p->zero_bytes = 0;
  p->swap_elem = swap_loc;
}
*/
