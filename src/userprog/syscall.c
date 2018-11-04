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

#define FOUR_INTS(esp) *(int*)((esp)+4), *(int*)((esp)+8), *(int*)((esp)+12), *(int*)((esp)+16)

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

  t->exit_status = status;

  sema_up(&t->sema_w); // unblock parent
  printf("%s: exit(%d)\n", name, status);
  sema_down(&t->sema_e); // wait for parent to receive exit status

  unblock_children(t); // unblock children that are waiting for parent

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
  if(pid == PID_ERROR) return PID_ERROR;

  // use waited flag to see if we have already waited for this pid
  // instead of old system that used a linked list of all waited pids.
  struct thread *child = find_child_by_tid((tid_t) pid);
  if(!child) return PID_ERROR;
  if(child->waited) return PID_ERROR;
  child->waited = 1;

  return process_wait((tid_t) pid);
}

bool
syscreate (const char *file, unsigned initial_size)
{
  bool result;

  if(!is_user_vaddr(file)) sysexit(-1);
  if(!file) sysexit(-1);

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
sysopen (const char *file)
{
  int fd;

  if(!is_user_vaddr(file)) sysexit(-1);
  if(!file) sysexit(-1);

  lock_acquire(&filelock);
  
  struct thread *curr = thread_current();
  struct file *file_ptr = filesys_open(file);
  if(file_ptr == NULL) fd = -1;
  else
    {
      fd = curr->fd_cnt + 2;
      curr->fd_cnt++;
      curr->files[fd-2] = file_ptr;
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
  int success = 0;

  if(fd == 0)
    {
      unsigned n = 0;
      while(n++ < size && (*((uint8_t *)buffer + n) = input_getc()));
      return n;
    }
  else
    {
      lock_acquire(&filelock);

      struct thread *curr = thread_current();
      struct file *file = curr->files[fd-2];
      if(!file) return -1;
      success = file_read(file, buffer, size);

      lock_release(&filelock);

      return success;
    }
  return -1;
}

int
syswrite (int fd, const void *buffer, unsigned size)
{
  if(!is_user_vaddr(buffer)) sysexit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, buffer+size)) sysexit(-1);

  int success = 0;

  if(fd == 1)
    {
      putbuf((char*) buffer, (size_t) size);
      return size;
    }
  else
    {
      lock_acquire(&filelock);

      struct thread *curr = thread_current();
      struct file *file = curr->files[fd-2];
      if(file) success = file_write(file, buffer, size);

      lock_release(&filelock);
    }
  return success;
}

void
sysseek(int fd, unsigned position)
{
  lock_acquire(&filelock);

  struct thread *curr = thread_current();
  struct file *file = curr->files[fd-2];
  file_seek(file, position);

  lock_release(&filelock);
}

unsigned
systell(int fd)
{
  lock_acquire(&filelock);

  struct thread *curr = thread_current();
  struct file *file = curr->files[fd-2];
  unsigned result;
  result = file_tell(file);

  lock_release(&filelock);

  return result;
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
      curr->files[fd-2] = NULL;
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
        f->eax = sysread(*(int*)(esp+4), *(void**)(esp+8), *(unsigned*)(esp+12)); break;
      case SYS_WRITE: 
        f->eax = syswrite(*(int*)(esp+4), *(const void**)(esp+8), *(unsigned*)(esp+12)); break;
      case SYS_SEEK: break;
      case SYS_TELL: break;
      case SYS_CLOSE: sysclose(*(int*)(esp+4)); break;
      /*project 1 additional system calls*/ 
      case SYS_PIBO: f->eax = pibonacci(*(int*)(esp+4)); break;
      case SYS_SUM: f->eax = sum_of_four_integers(FOUR_INTS(esp)); break;
      default: sysexit(-1);
    }
  /* end of 3.3.4 block */
}
