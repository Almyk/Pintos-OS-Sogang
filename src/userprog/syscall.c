#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* 3.3.4 code block */
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "userprog/process.h"
#include "lib/user/syscall.h"

struct lock filelock;
/* end of 3.3.4 block */

static void syscall_handler (struct intr_frame *);

/* 3.3.4 SystemCall -- Implementation of each functions */
void
syshalt (void)
{
  shutdown_power_off();
}

void sysexit (int status)
{
  struct thread *t = thread_current();
  t->status = status;
  printf("%s: exit(%d)\n", t->name, status);
  thread_exit();
}

pid_t sysexec (const char *cmd_line)
{
  return process_execute(cmd_line);
}

int syswait (pid_t pid)
{
  return process_wait(pid);
}

void syscreate(){
}

void sysremove(){
}

void sysopen(){
}

void sysfilesize(){
}

int sysread(int fd, void *buffer, unsigned size)
{
}

int syswrite (int fd, const void *buffer, unsigned size)
{
  struct file* f;
  printf("\n\nsyswrite syscall\n"); // remove: debugging purposes
  if(fd == 1)
    {
      putbuf((char*) buffer, (size_t) size);
      return size;
    }
  return 0;
}

void sysseek(){
}

void systell(){
}

void sysclose(){
}

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
      case SYS_HALT: syshalt(); break;
      case SYS_EXIT: sysexit(*(int*)(esp+4)); break;
      case SYS_EXEC: break;
      case SYS_WAIT: break;
      case SYS_CREATE: break;
      case SYS_REMOVE: break;
      case SYS_OPEN: break;
      case SYS_FILESIZE: break;
      case SYS_READ: break;
      case SYS_WRITE: 
        syswrite(*(int*)(esp+4), *(const void**)(esp+8), *(unsigned*)(esp+12)); break;
      case SYS_SEEK: break;
      case SYS_TELL: break;
      case SYS_CLOSE: break;
    }
  /* end of 3.3.4 block */
  printf ("system call!\n");
  thread_exit ();
}
