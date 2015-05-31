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
#include "devices/timer.h"
//
struct lock filesys_lock;

struct process_file{
	struct file * file;
	int fd;
	struct list_elem elem;
};

struct file* get_file_by_fd (int fd);
struct child_process * get_child_by_tid (int tid);
///
static void syscall_handler (struct intr_frame *);

typedef int pid_t; // khg : I don't know where it is.
// khg : proto type
static void my_halt(void);
//void my_exit(int status);
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
    
    char *temp = malloc(sizeof(char) * (length+1));
    int i, check = 0, c;
    c = file_read(fp, temp, length);

    for(i=0; i < c ; ++i)
    {
        if(temp[i]!=((char *)buffer)[i])
            check = 1;
    }

    if(check == 0) // same
    {
        lock_release(&filesys_lock);
        return c;
    }
    else
        file_seek(fp, file_tell(fp)-c);  

    
    int byte = file_write(fp, buffer, length);
	lock_release(&filesys_lock);
	return byte;
}

void
my_exit(int status)
{
    struct thread *cur = thread_current();
    printf("%s: exit(%d)\n", cur->pname, status);
    cur->cp->exit = status;
    cur->cp->wait = false;
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
    //printf("execute\n");
    if(!address_valid(cmd_line))
        my_exit(-1);

    struct thread *t = thread_current();
    //printf("process_execute start\n");
	tid_t tid = process_execute(cmd_line);
   	//printf("process_execute finish tid %d\n",tid);
    //printf("my_exec_new_tid : %d\n", tid);
    //printf("my_exec : %p, tid : %d, name : %s\n", t, t->tid, t->pname);
	struct child_process* cp = get_child_by_tid(tid);
	//printf("cp in exec : %p : load : %d\n", cp, cp->load);
    //printf("process_execute end tid: %d && cp->load : %d\n", tid, cp->load);

	 	while(cp->not_load == true)
        timer_sleep(1);
    if(cp->load == false)
        return -1;
   // printf("exec return tid: %d\n",tid);
    return tid;
	
}
static int
my_wait(pid_t pid)
{
    //printf("wait\n");
   	//printf("wait start  tid: %d\n",thread_current()->tid);
    tid_t tid = (tid_t) pid;
    process_wait(tid);
}
static bool
my_create(const char *file, unsigned initial_size)
{
	lock_acquire(&filesys_lock);
	
	if(file == NULL){
        lock_release(&filesys_lock);
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
	lock_acquire(&filesys_lock);
	
	if(file == NULL || !address_valid(file)){
		lock_release(&filesys_lock);
		return false;
	}

	bool success = filesys_remove(file);

	lock_release(&filesys_lock);
	return success;
}

typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

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

  struct Elf32_Ehdr ehdr;
  if (!(file_read (fp, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024)) 
    {
	file_deny_write (fp);
    }
    file_seek(fp, 0);
	


	struct process_file *pf = malloc(sizeof(struct process_file));
	pf->file = fp;
	pf->fd = fd;
	(thread_current()->fd)++;
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
	
	if(fp == NULL){
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
	while(e != list_end(&t->file_list)){
		pf = list_entry (e, struct process_file, elem);
		// khg : order is important
        e = list_next(e);
		if(fd == pf->fd || fd == CLOSE_ALL){
			file_allow_write(pf->file);
			file_close(pf->file);
			list_remove(&pf->elem);
			free(pf);
			count++;

            // khg : fd reset....
            //(t->fd)--;
			if(fd != CLOSE_ALL)
				break;

        }
	}
	
	lock_release(&filesys_lock);
	
	if(count == 0 && fd != CLOSE_ALL)
		my_exit(-1);
	
	return;
}

struct file * get_file_by_fd (int fd){
	struct thread *t = thread_current();
	struct list_elem *e = list_begin(&t->file_list);
	struct process_file *pf;
	while(e != list_end(&t->file_list)){
		pf = list_entry (e, struct process_file, elem);
		e = list_next(e);
		if(fd == pf->fd)
			return pf->file;
	}
	return NULL;
}
struct child_process * get_child_by_tid (int tid){
	struct thread *t = thread_current();
    //printf("child_process_t : %p, tid : %d, name : %s\n", t, t->tid, t->pname);
	struct list_elem *e = list_begin(&t->child_list);
	struct child_process * cp;
	while(e != list_end(&t->child_list)){
		cp = list_entry (e, struct child_process, elem);
		//printf("cp : %p : tid = %d current : %s\n",cp, cp->tid, t->pname);
        e = list_next(e);
		if(tid == cp->tid)
			return cp;
	}
	return NULL;
}
