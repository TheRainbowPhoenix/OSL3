#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>

pid_t pid;

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

void killit(int sig) {
	kill(pid, SIGTERM);
	exit(0);
}

int main(int argc, char ** argv) {

	if(argc<3) {
		printf("Except 2 arguments\n");
		return 1;
	}

	int t = ft_atoi(argv[1]);

	pid = fork();

	char ** cmd = &argv[2];

	signal(SIGALRM, killit);

	if (pid == -1)
		perror("fork error");
	else if (pid == 0) {
		alarm((t<=0)?1:t);
		execvp(argv[2], cmd);
		printf("Unknown command or invalid path : %s\n", argv[1]);
		/*waitpid(pid, NULL, 0);*/
	}

	return 0;
}
