#include "userprog/syscall.h"
#include "userprog/process.h"
#include "devices/shutdown.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "lib/kernel/console.h"
#include "threads/synch.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/off_t.h"

struct file
{
	struct inode *inode;    
	off_t pos;               
	bool deny_write;            
};

struct lock file_lock;

static void syscall_handler(struct intr_frame *);


void exit(int now_status) {
	printf("%s: exit(%d)\n", thread_name(), now_status);
	thread_current()->exit_status = now_status;
	for (int i = 3; i < 256; ++i)
		if (thread_current()->file_des[i] != NULL)
			close(i);
	thread_exit();
}

int write(int type, const void *buffer, unsigned size) {
	int a;
	lock_acquire(&file_lock);
	if (type == 1) {
		putbuf(buffer, size);
		lock_release(&file_lock);
		return size;
	}
	else if (type == 0 || type == 2){
		lock_release(&file_lock);
		return -1;
	}
	else{
		if (thread_current()->file_des[type] == NULL) exit(-1);
		if (thread_current()->file_des[type]->deny_write) {
			file_deny_write(thread_current()->file_des[type]);
		}
		a = file_write(thread_current()->file_des[type], buffer, size);
		lock_release(&file_lock);
		return a;
	}
}

int exec(const char *cmd_line) {
	return process_execute(cmd_line);
}

int read(int type, void* buffer, unsigned size) {
	int a;
	if (!is_user_vaddr(buffer)) exit(-1);
	lock_acquire(&file_lock);
	if (type == 0) {
		for (int i = 0; i < (int)size; ++i)
			if (((char *)buffer)[i] == '\0') {
				lock_release(&file_lock);
				return i;
			}

	}
	else if (type == 1 || type == 2){
		lock_release(&file_lock);
		return -1;
	}
	else{
		if (thread_current()->file_des[type] == NULL) exit(-1);
		a = file_read(thread_current()->file_des[type], buffer, size);
		lock_release(&file_lock);
		return a;
	}	
}

int fibonacci(int n) {
	if (n == 1 || n == 2) return 1;
	return fibonacci(n - 1) + fibonacci(n - 2);
}
int max_of_four_int(int a, int b, int c, int d) {
	int max = a;
	if (max < b) max = b;
	if (max < c) max = c;
	if (max < d) max = d;
	return max;
}

int open(const char *file) {
	if (file == NULL || !is_user_vaddr(file)) exit(-1);
	lock_acquire(&file_lock);
	struct file* fp = filesys_open(file);
	if (fp == NULL) {
		lock_release(&file_lock);
		return -1;
	}
	else {
		for (int i = 3; i < 256; i++)
			if (thread_current()->file_des[i] == NULL) {
				if (strcmp(thread_current()->name, file) == 0) {
					file_deny_write(fp);
				}
				thread_current()->file_des[i] = fp;
				lock_release(&file_lock);
				return i;
			}
	}
}

void close(int fd) {
	struct file* fp = thread_current()->file_des[fd];
	if (thread_current()->file_des[fd] == NULL) exit(-1);
	thread_current()->file_des[fd] = NULL;
	return file_close(fp);
}


void
syscall_init (void) 
{
	lock_init(&file_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	
  int *esp = f->esp;
  
  
  if(esp[0] == SYS_HALT){
	  shutdown_power_off();
  }
  else if(esp[0] == SYS_EXIT){
	  if (!is_user_vaddr(&esp[1])) {
		  exit(-1);
	  }
	  exit(*(&esp[1]));
  }
  else if(esp[0] == SYS_EXEC){
	  if (!is_user_vaddr(&esp[1])) {
		  exit(-1);
	  }
	  f->eax = exec((const char*)*(&esp[1]));
  }
  else if(esp[0] == SYS_WAIT){
	  if (!is_user_vaddr(&esp[1])) {
		  exit(-1);
	  }
	  f-> eax = process_wait((int)*(&esp[1]));
  }
  else if(esp[0] == SYS_CREATE){//22
	  if (!is_user_vaddr(&esp[4]) || !is_user_vaddr(&esp[5]) || (const char *)esp[4] == NULL) {
		  exit(-1);
	  }
	  f->eax = filesys_create((const char *)esp[4], (unsigned)esp[5]);
  }
  else if(esp[0] == SYS_REMOVE){//22
	  if (!is_user_vaddr(&esp[1]) || (const char*)esp[1] == NULL) exit(-1);
	  f->eax = filesys_remove((const char*)esp[1]);
  }
  else if(esp[0] == SYS_OPEN){//22
	  if (!is_user_vaddr(&esp[1])) exit(-1);
	  f->eax = open((const char*)esp[1]);
  }
  else if(esp[0] == SYS_FILESIZE){//22
	  if (!is_user_vaddr(&esp[1]) || thread_current()->file_des[(int)esp[1]] == NULL) exit(-1);
	  f->eax = file_length(thread_current()->file_des[(int)esp[1]]);
  }
  else if(esp[0] == SYS_READ){
	  if (!is_user_vaddr(&esp[5]) || !is_user_vaddr(&esp[6]) || !is_user_vaddr(&esp[7])) {
		  exit(-1);
	  };
	  f->eax = read((int)*(&esp[5]), (void *)*(&esp[6]), (unsigned)*(&esp[7]));
  }
  else if(esp[0] == SYS_WRITE){
	  f->eax = write((int)*(&esp[5]), (void *)*(&esp[6]), (unsigned)*(&esp[7]));
  }
  else if(esp[0] == SYS_SEEK){//22
	  if (!is_user_vaddr(&esp[4]) || !is_user_vaddr(&esp[5]) || thread_current()->file_des[(int)esp[4]] == NULL) exit(-1);
	  file_seek(thread_current()->file_des[(int)esp[4]], (unsigned)esp[5]);
  }
  else if(esp[0] == SYS_TELL){//22
	  if (!is_user_vaddr(&esp[1]) || thread_current()->file_des[(int)esp[1]] == NULL) exit(-1);
	  f->eax = file_tell(thread_current()->file_des[(int)esp[1]]);
  }
  else if(esp[0] == SYS_CLOSE){//22
	  if (!is_user_vaddr(&esp[1])) exit(-1);
	  close((int)esp[1]);
  }
  else if (esp[0] == SYS_FIBO) {
	  f->eax = fibonacci((int)*(&esp[1]));
  }
  else if (esp[0] == SYS_MAX4) {
	  f->eax = max_of_four_int((int)*(&esp[6]), (int)*(&esp[7]), (int)*(&esp[8]), (int)*(&esp[9]));
  }
}

