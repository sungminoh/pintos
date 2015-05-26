#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/init.h"


static void syscall_handler (struct intr_frame *);

typedef int pid_t; // khg : I don't know where it is.
// khg : proto type
static void my_halt(void);
static void my_exit(int status);
static pid_t my_exec(const char *cmd_line);
static int my_wait(pid_t pid); 
static bool my_create(const char *file, unsigned initial_size); 
static bool my_remove(const char *file);
static int my_open(const char *file);
static int my_filesize(int fd);
static int my_read(int fd, void *buffer, unsigned size);
static int my_write(int fd, const void *buffer, unsigned size);
static void my_seek(int fd, unsigned position);
static unsigned my_tell(int fd);
static int my_close(int fd);
//-----------------------------

// khg : function pointer && make table by syscall num
typedef int (*func_p) (int, int, int);
static func_p syscall_table[SYS_INUMBER+1] =
{
  (func_p)my_halt, (func_p)my_exit, (func_p)my_exec, (func_p)my_wait,
  (func_p)my_create, (func_p)my_remove, (func_p)my_open, (func_p)my_filesize,
  (func_p)my_read, (func_p)my_write, (func_p)my_seek, (func_p)my_tell,
  (func_p)my_close
};


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// khg : is address valid?
static bool
address_valid (void *address) 
{
  return (address < PHYS_BASE && pagedir_get_page (thread_current ()->pagedir, address) != NULL );
} //---------------------------


static void
syscall_handler (struct intr_frame *f) 
{
  
  int call_num, ret;
  int *esp = (int *)f->esp;
  // khg : valid check
  if(!address_valid((void *)esp))
  {
      my_exit(-1);
  }

  call_num = *esp;
  
  if(call_num < SYS_HALT || call_num > SYS_INUMBER)
  {
      my_exit(-1);
  }

  // I don't know how to check... more good.
  if(call_num == SYS_EXIT || call_num == SYS_EXEC || call_num == SYS_WAIT ||   
     call_num == SYS_OPEN || call_num == SYS_FILESIZE || call_num == SYS_TELL ||
     call_num == SYS_CLOSE || call_num == SYS_REMOVE)
  {
    if(!address_valid(esp + 1))
    {
        my_exit(-1);
    }
      
  }
  else if(call_num == SYS_CREATE || call_num == SYS_READ || call_num == SYS_SEEK)
  {
    if(!address_valid(esp + 1) || !address_valid(esp + 2))
    {
        my_exit(-1);
    }
  }
  else if(call_num == SYS_READ || call_num == SYS_WRITE)
  {
    if(!address_valid(esp + 1) || !address_valid(esp + 2) || !address_valid(esp + 3))
    {
       my_exit(-1);
    }
  }
  // -- end valid check
  
  func_p func;
  func = syscall_table[call_num];

  f->eax = func(*(esp + 1), *(esp + 2), *(esp + 3));
   return;


 //NOT_REACHED ();
 //  origin code 
 // thread_exit ();
}
// khg : syscalls -------------------------------
static int
my_write (int fd, const void *buffer, unsigned length)
{
  struct file * f;
  int ret;
 
 

  ret = -1;
  if (fd == STDOUT_FILENO) /* stdout */
      putbuf (buffer, length);
  else
  {
      /* after main.... there is somthing...
      printf("Not yet : write\n");
      my_exit(-1);
      */
      my_exit(0);
  }
  

  return ret;
}

static void
my_exit(int status)
{
    struct thread *cur = thread_current();
    printf("%s: exit(%d)\n", cur->pname, status);
    thread_exit();
}

static void
my_halt(void)
{
    shutdown_power_off();
}

static pid_t
my_exec(const char *cmd_line)
{
    printf("Not yet : exec\n");
    my_exit(-1);
}
static int
my_wait(pid_t pid)
{
    printf("Not yet : wait\n");
    my_exit(-1);
}
static bool
my_create(const char *file, unsigned initial_size)
{
    printf("Not yet : creat\n");
    my_exit(-1);
}
static bool
my_remove(const char *file)
{
    printf("Not yet : remove\n");
    my_exit(-1);
}
static int 
my_open(const char *file)
{
    printf("Not yet : open\n");
    my_exit(-1);
}
static int 
my_filesize(int fd)
{
    printf("Not yet : filesize\n");
    my_exit(-1);
}
static int 
my_read(int fd, void *buffer, unsigned size)
{
    printf("Not yet : read\n");
    my_exit(-1);
}
static void 
my_seek(int fd, unsigned position)
{
    printf("Not yet : seek\n");
    my_exit(-1);
}
static unsigned
my_tell(int fd)
{
    printf("Not yet : tell\n");
    my_exit(-1);
}
static int
my_close(int fd)
{
    printf("Not yet : close\n");
    my_exit(-1);
}
