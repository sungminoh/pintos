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

///
struct lock filesys_lock;

struct process_file{
	struct file * file;
	int fd;
	struct list_elem elem;
};

struct file* get_file_by_fd (int fd);

///
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
void my_close(int fd);
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
	lock_init(&filesys_lock);
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
	lock_acquire(&filesys_lock);
	if(!address_valid(buffer)){
		lock_release(&filesys_lock);
		my_exit(-1);
	}
  if (fd == STDOUT_FILENO){/* stdout */
  	putbuf (buffer, length);
		lock_release(&filesys_lock);
		return length;
	}
	struct file *fp = get_file_by_fd(fd);

	if(fp == NULL){
		lock_release(&filesys_lock);
		return -1;
	}

	int byte = file_write(fp, buffer, length);
	lock_release(&filesys_lock);
	return byte;
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
	lock_acquire(&filesys_lock);
	
	if(file == NULL){
		my_exit(-1);
	}
	if(!address_valid(file)){
		lock_release(&filesys_lock);
		my_exit(-1);
	}

	bool success = filesys_create(file, initial_size);
	lock_release(&filesys_lock);
	return success;
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
	lock_acquire(&filesys_lock);

	if(file == NULL){
		lock_release(&filesys_lock);
		return -1;
	}
	
	if(!address_valid(file)){
		lock_release(&filesys_lock);
		my_exit(-1);
	}

	struct file *fp = filesys_open(file);
	int fd = thread_current()->fd;

	if(!fp){
		lock_release(&filesys_lock);
  //  printf("file open error\n");
    return -1;
	}

	
	struct process_file *pf = malloc(sizeof(struct process_file));
	pf->file = fp;
	pf->fd = fd;
	thread_current()->fd++;
	list_push_back(&thread_current()->file_list, &pf->elem);

	lock_release(&filesys_lock);

	return fd;	
}
static int 
my_filesize(int fd)
{
	lock_acquire(&filesys_lock);
	struct file * fp = get_file_by_fd(fd);

	if(fp == NULL){
		lock_release(&filesys_lock);
		return -1;
	}
	lock_release(&filesys_lock);
	return file_length(fp);

}
static int 
my_read(int fd, void *buffer, unsigned size)
{
	lock_acquire(&filesys_lock);
	if(!address_valid(buffer)){
		lock_release(&filesys_lock);
		my_exit(-1);
	}
	if(fd == STDIN_FILENO){
		unsigned i = 0;
		uint8_t* local_buffer = (uint8_t *)buffer;
		for(;i< size; i++)
			local_buffer[i] = input_getc();
		lock_release(&filesys_lock);
		return size;
	}
	
	struct file * fp = get_file_by_fd(fd);
	
	if(!fp){
		lock_release(&filesys_lock);
		return -1;
	}
	
	int byte = file_read(fp, buffer, size);
	lock_release(&filesys_lock);
	return byte;

}
static void 
my_seek(int fd, unsigned position)
{
	lock_acquire(&filesys_lock);
	
	struct file *fp = get_file_by_fd(fd);
	
	if(fp =  NULL){
		lock_release(&filesys_lock);
		return;
	}
	file_seek(fp, position);
	lock_release(&filesys_lock);
	return;
}
static unsigned
my_tell(int fd)
{
	lock_acquire(&filesys_lock);
	
	struct file * fp = get_file_by_fd(fd);

	if(fd == NULL){
		lock_release(&filesys_lock);
		return -1;
	}

	off_t off = file_tell(fp);

	lock_release(&filesys_lock);
	return off;
}

void
my_close(int fd)
{
	lock_acquire(&filesys_lock);

	struct thread *t = thread_current();
	struct list_elem *e = list_begin(&t->file_list);
	struct process_file *pf;
	int count = 0;

	while(e != NULL){
		pf = list_entry (e, struct process_file, elem);
		if(fd == pf->fd || fd == CLOSE_ALL){
			file_close(pf->file);
			list_remove(&pf->elem);
			free(pf);
			count++;
			if(fd != CLOSE_ALL)
				break;
		}
		if(e == list_end(&t->file_list))
			break;	
		e = list_next(e);
	}
	
	lock_release(&filesys_lock);
	
	if(count == 0)
		my_exit(-1);
	
	return;
}

struct file * get_file_by_fd (int fd){
	struct thread *t = thread_current();
	struct list_elem *e = list_begin(&t->file_list);
	struct process_file *pf;
	while(e != NULL){
		pf = list_entry (e, struct process_file, elem);
		if(fd == pf->fd)
			return pf->file;
		if(e == list_end(&t->file_list))
			break;
		e = list_next(e);
	}
	return NULL;
}
