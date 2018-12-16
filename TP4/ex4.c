#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <errno.h>

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

int NbFichier2(char *repertoire) {
  int fd[2];
  int fd2[2];
  pid_t pid;

  if (pipe(fd)==-1) printerr("pipe error");
  if((pid = fork()) == -1) printerr("fork error");

  if(pid==0) {
    //Exec 1
    close(STDOUT_FILENO);
    dup(fd[1]);
    close(fd[0]);
    close(fd[1]);
    const char* args[] = { "ls",repertoire , 0};
    execvp(args[0], (char * const *)args);
    printerr("exec error");
  }

  if (pipe(fd2)==-1) printerr("pipe error");
  if ((pid = fork()) == -1) {
    printerr("for error [2]");
  }
  if(pid==0) {
    // fd --> wc -l
    //Exec 2
    close(STDIN_FILENO); //0
    dup(fd[0]);
    close(STDOUT_FILENO); //1
    dup(fd2[1]);
    close(fd[0]);
    close(fd[1]);
    close(fd2[0]);
    close(fd2[1]);
    const char* args[] = { "wc", "-l" , 0};
    execvp(args[0], (char * const *)args);
    printerr("exec error");
  }

  close(fd[0]);
  close(fd[1]);

  close(fd2[1]); //0 ?
  int rc;
  char buffer;
  int i = -1;
  char input[UCHAR_MAX]; //TODO : free !
  while((rc = read(fd2[0], &buffer, 1)) && i++<UCHAR_MAX-1 && buffer != '\0') {
    input[i] = buffer;
  }
  input[i] = '\0';
  return ft_atoi(input);
  wait(NULL);

  return 0;
}

void NbFichier(char *repertoire) {
  int fd[2];
  pid_t pid;

  if (pipe(fd)==-1) printerr("pipe error");
  if((pid = fork()) == -1) printerr("fork error");

  if(pid==0) {
    //Exec 1
    close(STDOUT_FILENO);
    dup(fd[1]);
    close(fd[0]);
    close(fd[1]);
    const char* args[] = { "ls",repertoire , 0};
    execvp(args[0], (char * const *)args);
    printerr("exec error");
  }

  if ((pid = fork()) == -1) {
    printerr("for error [2]");
  }
  if(pid==0) {
    // fd --> wc -l
    //Exec 2
    close(STDIN_FILENO);
    dup(fd[0]);
    close(fd[0]);
    close(fd[1]);
    const char* args[] = { "wc", "-l" , 0};
    execvp(args[0], (char * const *)args);
    printerr("exec error");
  } else {
    close(fd[1]);
    /*int rc;
    char buffer;
    int i = -1;
    char input[UCHAR_MAX]; //TODO : free !
    while((rc = read(fd[0], &buffer, 1)) && i++<UCHAR_MAX-1 && buffer != '\0') {
      input[i] = buffer;
    }
    input[i] = '\0';

    printf("%s\n", input);*/
    wait(NULL);
  }
  wait(NULL);


}

void LsDansFichier(char *repertoire, char *fichierResultat) {
  int fd[2];
  pid_t pid;

  if (pipe(fd)==-1) printerr("pipe error");
  if((pid = fork()) == -1) printerr("fork error");

  if(pid==0) {
    close(STDOUT_FILENO);
    dup(fd[1]);
    close(fd[0]);
    close(fd[1]);

    const char* args[] = { "ls",repertoire , 0};
    execvp(args[0], (char * const *)args);
    printerr("exec error");
  } else {
    close(fd[1]);
    int rc;
  	char buffer;
  	int i = -1;
    char input[UCHAR_MAX]; //TODO : free !
  	while((rc = read(fd[0], &buffer, 1)) && i++<UCHAR_MAX-1 && buffer != '\0') {
  		input[i] = buffer;
  	}
  	input[i] = '\0';

    FILE * fp;
    fp = fopen(fichierResultat, "w+");
    fprintf(fp, "%s", input);

    //printf("%s\n", input);
    wait(NULL);
  }
}

void Ls(char *repertoire) {
  int fd[2];
  pid_t pid;

  if (pipe(fd)==-1)
    printerr("pipe error");

  if((pid = fork()) == -1) {
    printerr("fork error");
  }
  if(pid==0) {
    close(STDOUT_FILENO);
    dup(fd[1]);
    close(fd[0]);
    close(fd[1]);

    const char* args[] = { "ls",repertoire , 0};
    execvp(args[0], (char * const *)args);
    printerr("exec error");
  } else {
    close(fd[1]);
    int rc;
  	char buffer;
  	int i = -1;
    char input[UCHAR_MAX]; //TODO : free !
  	while((rc = read(fd[0], &buffer, 1)) && i++<UCHAR_MAX-1 && buffer != '\0') {
  		input[i] = buffer;
  	}
  	input[i] = '\0';
    printf("%s\n", input);
    wait(NULL);
  }

}

int main(int argc, char const *argv[]) {
  Ls("..");
  LsDansFichier("..","f1.txt");
  NbFichier(".");
  printf("Line count : %d\n", NbFichier2(".."));
  return 0;
}
