#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

int run = 1;
int sw = 0;
pid_t pid1;
pid_t pid2;

void tac(int sig) {
	printf("Sig %d !\n", getpid());
}

void tick(int sig) {
	if(sw == 0) {
			kill(pid1, SIGUSR1);
			tac(0);
			sw = 1;
		} else {
			kill(pid2, SIGUSR1);
			sw=0;
		}
		alarm(2);
}

int main() {
	
	pid1 = fork();
	pid2 = fork();
	
	if (pid1 == -1 || pid2 == -1)
		perror("fork error");

		signal(SIGUSR1, tac);
		signal(SIGALRM, tick); 
	
	alarm(2);
	
	printf("pid %d %d\n", getppid(), getpid());
	
	while(run) {
		pause();
	}		
	
	return 0;
}
