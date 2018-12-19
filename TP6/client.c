#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

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

void printerr(char err[]) {
  printf("%s : %d\n",err, errno);
  exit(EXIT_FAILURE);
}

pid_t pid=0;

int fd[2];
int r[2];

void Nettoyer() {
  if(pid != 0) kill(pid, SIGTERM);
}

void tick(int sig) {
  Nettoyer();
  alarm(180);
}

void create() {
  if (pipe(fd)==-1) printerr("pipe error");
  if (pipe(r)==-1) printerr("pipe error (2)");
  if((pid = fork()) == -1) printerr("fork error");
  if(pid==0) {
    close(fd[1]);
    close(r[0]);
    close(STDIN_FILENO);
    dup(fd[0]);
    close(STDOUT_FILENO);
    dup(r[1]);
    close(fd[0]);
    close(r[1]);

    const char* args[] = { "bc" , 0};
    execvp(args[0], (char * const *)args);
    printerr("exec error");
  } else {
    close(fd[0]);
    close(r[1]);
  }
}

void end() {
  close(fd[1]);
  close(r[0]);
}

int Operation(char operateur, int operande1, int operande2) {
  signal(SIGALRM, SIG_DFL);

  char buffer[UCHAR_MAX];
  int a = operande1;
  int b = operande2;
  int c = operateur;
  //int a = 7;
  //int b = 6;
  //int c = '*';
  int l = sprintf(buffer, "%d%c%d\n",a,c,b);
  write(fd[1], buffer, l*sizeof(char));

  char rtrn[16];
  int nbytes = read(r[0], rtrn, sizeof(rtrn));
  if (nbytes <= 0) {
    fprintf(stderr, "read from child failed\n");
    return 0;
  } else return ft_atoi(rtrn);
  signal(SIGALRM, tick);
  return 0;
}

int main(int argc, char const *argv[]) {
  alarm(180);
  signal(SIGALRM, tick);
  create();

  printf("%d\n", Operation('*',7,6));
  printf("%d\n", Operation('+',2,2));
  printf("%d\n", Operation('-',3,1));
  end();
  Nettoyer();
  return 0;
}
