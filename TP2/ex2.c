#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>

int main(int argc, char ** argv) {
	
	pid_t pid = fork();
	if(argc<2) return 0;
	
	char ** cmd = &argv[1];	

	if (pid == -1)
		perror("fork error");
	else if (pid == 0) {
		execvp(argv[1], cmd);
		printf("Unknown command or invalid path : %s\n", argv[1]);
		waitpid(pid, NULL, 0);
	}

	return 0;
}
