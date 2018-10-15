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
#include "userprog/pagedir.h"

struct list wait_child_list;
typedef struct waitPid{
  pid_t pid;
  struct list_elem wpelem;
};
/* end of 3.3.4 block */

static void syscall_handler (struct intr_frame *);

/* 3.3.4 SystemCall -- Implementation of each functions */
void
syshalt (void)
{
  shutdown_power_off();
}

void
sysexit (int status)
{
  int i = 0;
  struct thread *t = thread_current();
  char name[sizeof(t->name)+1];

  while((name[i] = t->name[i]) != ' ') i++;
  name[i] = '\0';

  t->parent->child_exit_status = status;
  t->parent->waiting -= 1;
  printf("%s: exit(%d)\n", name, status);
  thread_exit();
}

pid_t
sysexec (const char *cmd_line)
{
  if(!is_user_vaddr(cmd_line)) sysexit(-1);
  return process_execute(cmd_line);
}

int
syswait (pid_t pid)
{
  struct list_elem *e;

  if(pid == PID_ERROR) return pid;

  for(e = list_begin(&wait_child_list);
      e != list_end(&wait_child_list);
      e = list_next(e))
    {
      if(list_entry(e, struct waitPid, wpelem)->pid == pid)
        return PID_ERROR;
    }

  struct waitPid * new = palloc_get_page (0);
  new->pid = pid;
  list_push_back(&wait_child_list, &new->wpelem);
  return process_wait((tid_t) pid);
}

void syscreate(){
}

void sysremove(){
}

void sysopen(){
}

void sysfilesize(){
}

int
sysread (int fd, void *buffer, unsigned size)
{
  if(!is_user_vaddr(buffer+size)) sysexit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, buffer+size)) sysexit(-1);

  if(fd == 0)
    {
      unsigned n = 0;
      while(n++ < size && (*((uint8_t *)buffer + n) = input_getc()));
      return n;
    }
  return -1;
}

int
syswrite (int fd, const void *buffer, unsigned size)
{
  if(!is_user_vaddr(buffer)) sysexit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, buffer+size)) sysexit(-1);

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
  list_init(&wait_child_list);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}
/*project1 additional system calls*/
int
pibonacci(int n)
{
  
  int i;
  int f1 = 0;
  int f2 = 1;
  int fn;
    
  if(n == 1)
    return 0;
  if(n == 2)
    return 1;

  for(i=3; i<= n; i++)
    {
      fn = f1+ f2;
      f1 = f2;
      f2 = fn;
    }
  return fn;
}

int
sum_of_four_integers(int a, int b, int c, int d)
{
  return a+b+c+d;
}

static void
syscall_handler (struct intr_frame *f) 
{
  /* 3.3.4 System Calls code block */
  
  // check if pointer is valid or not
  if (f->esp == NULL || !is_user_vaddr(f->esp)) sysexit(-1);
  if (!pagedir_get_page(thread_current()->pagedir, f->esp)) sysexit(-1);

  void *esp = f->esp;
  int sysnum = *(int*)esp;
  if(sysnum != SYS_HALT) if(!is_user_vaddr(esp+4)) sysexit(-1);

  // remove : for debugging only
  //printf("\n--- syscall_handler ---\n\n");
  //printf("esp value: %x\n", esp);
  //printf("sysnum : %d\n", sysnum);
  //hex_dump(esp, esp, 0x80, 1);

  // make system call
  switch(sysnum)
    {
      case SYS_HALT: syshalt(); break;
      case SYS_EXIT: sysexit(*(int*)(esp+4)); break;
      case SYS_EXEC: f->eax = sysexec(*(const char**)(esp+4)); break;
      case SYS_WAIT: f->eax = syswait(*(int*)(esp+4)); break;
      case SYS_CREATE: break;
      case SYS_REMOVE: break;
      case SYS_OPEN: break;
      case SYS_FILESIZE: break;
      case SYS_READ:
        f->eax = sysread(*(int*)(esp+4), *(const void**)(esp+8), *(unsigned*)(esp+12)); break;
      case SYS_WRITE: 
        f->eax = syswrite(*(int*)(esp+4), *(const void**)(esp+8), *(unsigned*)(esp+12)); break;
      case SYS_SEEK: break;
      case SYS_TELL: break;
      case SYS_CLOSE: break;
      /*project 1 additional system calls*/ 
      case SYS_PIBO: f->eax = pibonacci(*(int*)(esp+4)); break;
      case SYS_SUM: f->eax = sum_of_four_integers(*(int*)(esp+4), *(int*)(esp+8), *(int*)(esp+12), *(int*)(esp+16)); break;
           
      default: sysexit(-1);
    }
  /* end of 3.3.4 block */
}
