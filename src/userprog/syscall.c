#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  /* 3.3.4 System Calls code block */
  void *esp = f->esp;
  uint32_t *pd = thread_current ()->pagedir;
  int sysnum = *(int*)esp;

  if (!is_user_vaddr(esp) || esp == NULL)
    {
      pagedir_destroy (pd);
      thread_exit ();
    }
  printf("\n--- syscall_handler ---\n\n");
  printf("esp value: %x\n", esp);
  printf("sysnum : %d\n", sysnum);
  hex_dump(esp, esp, 0x80, 1);
  /* end of 3.3.4 block */

  printf ("system call!\n");
  thread_exit ();
}
