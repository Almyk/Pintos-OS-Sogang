#include "vm/frame.h"

static struct hash ft;
static struct lock ft_lock;

void
ft_init (void)
{
  hash_init(&ft, ft_hash, ft_less, NULL);
  lock_init(&ft_lock);
}

unsigned
ft_hash (const struct hash_elem *e, void *aux)
{
  return hash_int((unsigned)hash_entry(e, struct fte, elem)->kaddr);
}

bool
ft_less (const struct hash_elem *a, const struct hash_elem *b, void *aux)
{
  return hash_entry(a, struct fte, elem)->kaddr
    < hash_entry(b, struct fte, elem)->kaddr;
}

uint8_t *
allocate_frame (void *upage, enum palloc_flags flag)
{
  uint8_t *knpage = palloc_get_page (flag);

  if(knpage == NULL)
    {
      knpage = evict_frame(upage, thread_current ());
    }
  else
    {
      struct fte *frame = (struct fte *) malloc (sizeof(struct fte));
      frame->vaddr = upage;
      frame->kaddr = knpage;
      frame->thread = thread_current();

      lock_acquire(&ft_lock);
      hash_insert(&ft, &frame->elem);
      lock_release(&ft_lock);
    }
  return knpage;
}

void *
evict_frame (void *upage, struct thread *t)
{
  struct hash_iterator it;
  uint32_t *pagedir = t->pagedir;
  void *knpage = NULL;
  int i;

  /* second chance algorithm */
  for(i = 0; i < 2 && !knpage; i++)
    {
      hash_first (&it, &ft);
      // look for not accessed and not dirty frame
      while(hash_next (&it) && !knpage)
        {
          struct fte *frame = hash_entry (hash_cur (&it), struct fte, elem);
          if(frame->thread == t)
            {
              uint8_t pte_flags = get_pte_flags (pagedir, upage);

              if(pte_flags == 1)
                {
                  // TODO: remove previous frame, swap??
                  knpage = frame->kaddr;
                }
            }
        }

      hash_first (&it, &ft);
      // look for not accessed but dirty frame
      while(hash_next (&it) && !knpage)
        {
          struct fte *frame = hash_entry (hash_cur (&it), struct fte, elem);
          if(frame->thread == t)
            {
              uint8_t pte_flags = get_pte_flags (pagedir, upage);

              if(pte_flags == 2)
                {
                  // TODO: remove previous frame, swap??
                  knpage = frame->kaddr;
                }
              pagedir_set_accessed (pagedir, upage, false);
            }
        }
    }

  return knpage;
}

uint8_t
get_pte_flags(uint32_t pagedir, void *page)
{
  bool accessed = pagedir_is_accessed (pagedir, page);
  bool dirty = pagedir_is_dirty (pagedir, page);

  if(!accessed && !dirty) return 1; 
  if(!accessed && dirty) return 2;
  if(accessed && !dirty) return 3;
  if(accessed && dirty) return 4;
}
