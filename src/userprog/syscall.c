#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "filesys/filesys.h"


static void syscall_handler (struct intr_frame *);

// khg : proto type
//static void my_halt(void);
static void my_exit(int status);
//static pid_t my_exec(const char *cmd_line);
//static int my_wait(pid_t pid);
//static bool my_create(const char *file, unsigned initial_size);
//static bool my_remove(const char *file);
//static int my_open(const char *file);
//static int my_filesize(int fd);
//static int my_read(int fd, void *buffer, unsigned size);
static int my_write(int fd, const void *buffer, unsigned size);
//static void my_seek(int fd, unsigned position);
//static unsigned my_tell(int fd);
//static int my_close(int fd);
//-----------------------------

static struct lock file_lock;


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// khg : is address valid?
static bool
address_valid (void *address) 
{
  return (address < PHYS_BASE  && pagedir_get_page (thread_current ()->pagedir, address) != NULL);
} //---------------------------


static void
syscall_handler (struct intr_frame *f) 
{
  
  int call_num, ret;
  int *esp = (int *)f->esp;
  
  if(!address_valid((void *)esp))
      my_exit(-1);

  call_num = *esp;
  
  if(call_num < SYS_HALT || call_num > SYS_INUMBER)
    my_exit(-1);

  if(call_num == SYS_WRITE)
    if(address_valid(esp + 1) && address_valid(esp + 2) && address_valid(esp + 3))
    {
      f->eax = my_write(*(esp + 1), *(esp + 2), *(esp + 3));
      return;
    }
    else
      my_exit(-1);
  else
    my_exit(0);


 //NOT_REACHED ();
 //  origin code 
 // thread_exit ();
}

static int
my_write (int fd, const void *buffer, unsigned length)
{
  struct file * f;
  int ret;
 
 

  ret = -1;
  if (fd == STDOUT_FILENO) /* stdout */
      putbuf (buffer, length);
  else
      my_exit(-1);

  

  return ret;
}

static void
my_exit(int status)
{
 putbuf("exit", 4); 
    thread_exit();
}
