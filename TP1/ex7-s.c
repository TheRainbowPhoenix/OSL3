#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <malloc.h>

void sighup(); /* routines child will call upon sigtrap */
void sigint();
void sigquit();

int	ft_atoi(char *str)
{
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
	while (*str >= '0' && *str <= '9')
	{
		rslt = rslt * 10 + *str - '0';
		str++;
	}
	return (sgn * rslt);
}


int main(int argc, char const *argv[]) {

  if(argc!=3) {
		printf("Invalid argument count : got %d expected 2\n", argc-1);
		return 1;
	}

  int * PIDs = (int *)malloc(2 * sizeof(int));
  PIDs[0] = ft_atoi(argv[1]);
  PIDs[1] = ft_atoi(argv[2]);
  int run = 1;

  while(run) {
    sleep(1);
    kill(PIDs[0], SIGUSR1);
    sleep(1);
    kill(PIDs[1], SIGUSR1);

  }

  free(PIDs);

  /* code */
  return 0;
}
