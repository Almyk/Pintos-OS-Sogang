#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void sysexit(int);

int pibonacci(int);
int sum_of_four_integers(int, int, int, int);

#endif /* userprog/syscall.h */
