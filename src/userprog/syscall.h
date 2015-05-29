#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#define CLOSE_ALL -2

void syscall_init (void);

struct child_process * get_child_by_tid (int tid);

void my_exit(int);

#endif /* userprog/syscall.h */
