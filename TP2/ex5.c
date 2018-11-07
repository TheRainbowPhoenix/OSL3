#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

int run = 1;
int sw = 0;
pid_t pid;

void tac(int sig) {
	printf("Signal @%d !\n", getpid());
}

void tick(int sig) {
	if(pid <= 0 || sw == 0) {
			tac(0);
			sw = 1;
		} else {
			kill(pid, SIGUSR1);
			sw=0;
		}
		alarm(2);
}

int main() {
	
	pid = fork();
	
	if (pid == -1)
		perror("fork error");
	
	if(pid>0) {
		alarm(2);
		signal(SIGALRM, tick); 
	}
	signal(SIGUSR1, tac);
	
	//printf("pid (%d) %d %d\n", pid, getppid(), getpid());
	
	while(run) {
		pause();
	}		
	
	return 0;
}
