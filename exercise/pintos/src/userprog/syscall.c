#include "userprog/syscall.h"
#include "userprog/process.h"
#include "devices/shutdown.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "lib/kernel/console.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);


void exit(int now_status) {
	printf("%s: exit(%d)\n", thread_name(), now_status);
	thread_current()->exit_status = now_status;
	thread_exit();
}

int write(int type, const void *buffer, unsigned size) {
	if (type == 1) {
		putbuf(buffer, size);
		return size;
	}
	return -1;
}

int exec(const char *cmd_line) {
	return process_execute(cmd_line);
}

int read(int type, void* buffer, unsigned size) {
	if (type == 0) 
		for (int i = 0; i < (int)size; ++i)
			if (((char *)buffer)[i] == '\0') return i;
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


void
syscall_init (void) 
{
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
  else if(esp[0] == SYS_CREATE){
  }
  else if(esp[0] == SYS_REMOVE){
  }
  else if(esp[0] == SYS_OPEN){
  }
  else if(esp[0] == SYS_FILESIZE){
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
  else if(esp[0] == SYS_SEEK){
  }
  else if(esp[0] == SYS_TELL){
  }
  else if(esp[0] == SYS_CLOSE){
  }
  else if (esp[0] == SYS_FIBO) {
	  f->eax = fibonacci((int)*(&esp[1]));
  }
  else if (esp[0] == SYS_MAX4) {
	  f->eax = max_of_four_int((int)*(&esp[6]), (int)*(&esp[7]), (int)*(&esp[8]), (int)*(&esp[9]));
  }
}

