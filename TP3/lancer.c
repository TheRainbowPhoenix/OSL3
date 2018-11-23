/**
 * @Author: uapv1701795
 * @Date:   2018-11-23T16:20:28+01:00
 * @Last modified by:   uapv1701795
 * @Last modified time: 2018-11-23T17:38:24+01:00
 */



#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

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

pid_t getOpid() {
	FILE* fp = fopen ("ordo.dat", "r");
	pid_t p = -1;
	fscanf(fp, "%d", &p);
	while(!feof(fp)) {
		printf("%d ", p);
		fscanf(fp, "%d", &p);
	}
	return p;
}

void write_pid(pid_t pid) {
  FILE * fp;
  fp = fopen("pid.dat","w+");
  fprintf(fp, "%d\n",pid);
  fclose(fp);
}

int main(int argc, char const *argv[]) {
  if(argc<2) {
		printf("Except at least 1 argument\n");
		return 1;
	}

	//pid_t pidOrdo = ft_atoi(argv[1]);
	pid_t pidOrdo = getOpid();

	pid = fork();
  int status;

	if (pid == -1)
		perror("fork error");
	else if (pid == 0) {
		execvp(argv[1], (char * const *)&argv[1]);
		printf("Unknown command or invalid path : %s\n", argv[1]);
		/*waitpid(pid, NULL, 0);*/
	} else {
		write_pid(pid);
    kill(pidOrdo, SIGUSR1);
		waitpid(pid, NULL, 0);
    //wait(&status);
    kill(pidOrdo, SIGUSR2);
  }

	return 0;
}
