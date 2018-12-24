#ifndef __FRAME__
#define __FRAME__

#include <hash.h>
#include "vm/page.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"

struct fte
  {
    uint8_t *vaddr; // virtual address, user space
    uint8_t *kaddr; // physical address, kernel space
    struct thread *thread; // thread that frame belongs to
    struct hash_elem elem;
  };

void ft_init (void);
unsigned ft_hash (const struct hash_elem *e, void *aux);
bool ft_less (const struct hash_elem *a, const struct hash_elem *b, void *aux);
uint8_t * allocate_frame (void *upage, enum palloc_flags flag);
void * evict_frame (void *upage, struct thread *t);
uint8_t get_pte_flags(uint32_t pagedir, void *page);
#endif
