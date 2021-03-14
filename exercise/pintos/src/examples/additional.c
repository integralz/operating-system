#include <stdio.h>
#include <syscall.h>
#include <stdlib.h>

int main(int args, char* argv[]){
	if(args == 5){
		printf("%d ",fibonacci(atoi(argv[1])));
		printf("%d\n",max_of_four_int(atoi(argv[1]),atoi(argv[2]),atoi(argv[3]),atoi(argv[4])));
	}
	return EXIT_SUCCESS;
}