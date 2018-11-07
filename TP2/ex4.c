#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>

pid_t pid = -2;
int run = 1;
int t;

char ** cmd;

int	ft_atoi(char *str) {
	int rslt;
	int sgn;

	rslt = 0;
	sgn = 1;
	while (*str == ' ' || (*str >= 9 && *str <= 13))
		str++;
	if (*str == '-')
		sgn = -1;
	if (*str == '-' || *str == '+')
		str++;
	while (*str >= '0' && *str <= '9') {
		rslt = rslt * 10 + *str - '0';
		str++;
	}
	return (sgn * rslt);
}

void tick(int sig) {
	if(pid != -2) {
		kill(pid, SIGTERM);	
	}
	pid = fork();

	if (pid == -1)
		perror("fork error");
	else if (pid == 0) {
		execvp(cmd[0], cmd);
		printf("Unknown command or invalid path : %s\n", cmd[0]);
	}
	alarm((t<=0)?1:t);
}

int main(int argc, char ** argv) {

	if(argc<3) {
		printf("Except 2 arguments\n");
		return 1;
	}

	t = ft_atoi(argv[1]);

	cmd = &argv[2];	

	signal(SIGALRM, tick);  
	alarm((t<=0)?1:t);
	while(run) {
		pause();
	}		

	return 0;
}
