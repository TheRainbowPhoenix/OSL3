#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

pid_t pid;
int run = 1;


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
	run = 0;
}

int main(int argc, char * argv[])
{
	if(argc != 3) {
		printf("Missing parametter : excpected 2 got %d\n",argc-1);
		return 1;
	}
	int t = ft_atoi(argv[2]);
	pid = ft_atoi(argv[1]);
	signal(SIGALRM, killit);  
	alarm((t<=0)?1:t);
	while(run) {
		pause();
	}
	return 0;
} 
