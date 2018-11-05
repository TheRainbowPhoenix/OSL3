#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main() {
	pid_t pid;
	pid = fork();

	if(pid == 0) {
		printf("child %d of %d\n", getpid(), getppid());
	} else if(pid>0) {
		printf("parent %d of %d\n", getpid(), pid);
	} else {
		perror("Fork error");
	}
	return 0;
}
