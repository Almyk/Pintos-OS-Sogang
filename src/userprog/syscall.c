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

#include "filesys/filesys.h"
#include "filesys/file.h"

struct list wait_child_list;
typedef struct waitPid{
  pid_t pid;
  struct list_elem wpelem;
};

struct lock filelock;

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

bool
syscreate (const char *file, unsigned initial_size)
{
  bool result;

  if(!is_user_vaddr(file)) sysexit(-1);

  lock_acquire(&filelock);
  result = filesys_create (file, initial_size);
  lock_release(&filelock);
  return result;
}

bool
sysremove (const char *file)
{
  bool result;

  if(!is_user_vaddr(file)) sysexit(-1);

  lock_acquire(&filelock);
  result = filesys_remove (file);
  lock_release(&filelock);
  return result;
}

int
sysopen(const char *file)
{
  int fd;

  if(!is_user_vaddr(file)) sysexit(-1);

  lock_acquire(&filelock);
  
  struct thread *curr = thread_current();
  struct file *file_ptr = filesys_open(file);
  if(!file_ptr) fd -1;
  else
    {
      fd = curr->fd_cnt + 2;
      curr->fd_cnt++;
      curr->files[fd] = file_ptr;
    }

  lock_release(&filelock);

  return fd;
}

int
sysfilesize (int fd)
{
  struct file *file;
  int result;

  lock_acquire(&filelock);

  struct thread *curr = thread_current();
  file = curr->files[fd-2];
  if(!file) result = -1;
  else result = (int) file_length (file);

  lock_release(&filelock);

  return result;
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
  // TODO: handle files
  else
    {
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
  // TODO: handle files
  else
    {
    }
  return 0;
}

void sysseek(){
}

void systell(){
}

void
sysclose (int fd)
{
  lock_acquire(&filelock);

  struct thread *curr = thread_current();
  struct file *file = curr->files[fd-2];
  if(file)
    {
      file_close(file);
    }

  lock_release(&filelock);
}

/*project1 additional system calls*/
int
pibonacci(int n)
{
  int i;
  int f1 = 0;
  int f2 = 1;
  int fn = 0;
    
  if(n == 1) return 1;
  if(n == 2) return 1;

  for (i = 2; i <= n; i++)
    {
      fn = f1 + f2;
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

void
syscall_init (void) 
{
  list_init(&wait_child_list);
  lock_init(&filelock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
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
      case SYS_CREATE: f->eax = syscreate(*(const char**)(esp+4), *(unsigned*)(esp+8)); break;
      case SYS_REMOVE: f->eax = sysremove(*(const char**)(esp+4)); break;
      case SYS_OPEN: f->eax = sysopen(*(const char**)(esp+4)); break;
      case SYS_FILESIZE: f->eax = sysfilesize(*(int*)(esp+4)); break;
      case SYS_READ:
        f->eax = sysread(*(int*)(esp+4), *(const void**)(esp+8), *(unsigned*)(esp+12)); break;
      case SYS_WRITE: 
        f->eax = syswrite(*(int*)(esp+4), *(const void**)(esp+8), *(unsigned*)(esp+12)); break;
      case SYS_SEEK: break;
      case SYS_TELL: break;
      case SYS_CLOSE: sysclose(*(int*)(esp+4)); break;
      /*project 1 additional system calls*/ 
      case SYS_PIBO: f->eax = pibonacci(*(int*)(esp+4)); break;
      case SYS_SUM: f->eax = sum_of_four_integers(*(int*)(esp+4), *(int*)(esp+8), *(int*)(esp+12), *(int*)(esp+16)); break;
           
      default: sysexit(-1);
    }
  /* end of 3.3.4 block */
}
