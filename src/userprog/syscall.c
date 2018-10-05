#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* 3.3.4 code block */
#include "threads/vaddr.h"
#include "threads/synch.h"

struct lock filelock;
/* end of 3.3.4 block */

static void syscall_handler (struct intr_frame *);
void syshalt(void);
void sysexit(int status);


void
syscall_init (void) 
{
  lock_init(&filelock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  /* 3.3.4 System Calls code block */
  void *esp = f->esp;
  uint32_t *pd = thread_current ()->pagedir;
  int sysnum = *(int*)esp;

  // check if pointer is valid or not
  if (!is_user_vaddr(esp) || esp == NULL)
    {
      pagedir_destroy (pd);
      thread_exit ();
    }

  // remove : for debugging only
  printf("\n--- syscall_handler ---\n\n");
  printf("esp value: %x\n", esp);
  printf("sysnum : %d\n", sysnum);
  hex_dump(esp, esp, 0x80, 1);

  // make system call
  switch(sysnum)
    {
      case SYS_HALT:
               syshalt();  
               break;
      case SYS_EXIT: 
               sysexit(); 
               break;
      case SYS_EXEC: break;
      case SYS_WAIT: break;
      case SYS_CREATE: break;
      case SYS_REMOVE: break;
      case SYS_OPEN: break;
      case SYS_FILESIZE: break;
      case SYS_READ: break;
      case SYS_WRITE: 
        lock_acquire(&filelock);
        printf("Hello, World!\n");
        lock_release(&filelock);
        break;
      case SYS_SEEK: break;
      case SYS_TELL: break;
      case SYS_CLOSE: break;
    }
  /* end of 3.3.4 block */
  printf ("system call!\n");
  thread_exit ();
}

/* 3.3.4 SystemCall -- Implementation of each functions */
void syshalt(){

  shutdown_power_off();

}

void sysexit(int status){
//still implementing...
}



