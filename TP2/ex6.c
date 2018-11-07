#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>

int run = 1;

char **ENV;

/* Temporary confing vars */

int SHOW_CWD = 1;
int SHOW_NAME = 1;
int SHOW_GIT = 1;

struct cmd {
  const char **argv;
};

void putstr(char *s) {
  while (*s) {
    write(1, &*s++, 1);
  }
}

void prompt() {
  char prompt[UCHAR_MAX];
  if(SHOW_NAME) {
    char	hostname[UCHAR_MAX];
    gethostname(hostname, UCHAR_MAX);
    printf("[32m[49m%s[37m[49m ",hostname);
  }
  if(SHOW_CWD) {
    char	cwd[UCHAR_MAX];
    if (getcwd(cwd, UCHAR_MAX) != NULL) {
      printf("[33m[49m%s[37m[49m ",cwd);
    }
  }
  if(SHOW_GIT) {
    char	cwd[UCHAR_MAX];
    /*
    ≡ No changes
    ↓ commit waiting to pull
    ↑ commit waiting to push
     */
    printf("[47m[90m%s%s%s%s%s[37m[49m ", "[", "master"," ≡ ", "+1", "]");
  }
  printf("\n");
  putstr("[90m>[37m[m "); /* TODO: Prompt custom */

}

void terminate() {
  printf("[39m[49m\n");
  exit(0);
}

void handler(int sig) {
  if(sig == SIGINT) {
    printf("\n");
    terminate();
    //signal(SIGINT, handler);
  }
}

int exec(int fd[2], struct cmd *line) {
  pid_t pid;
  char buffer[UCHAR_MAX];
  if((pid=fork()) == -1) return -1;
  if(pid == 0) {
    close(fd[0]);
    return execvp(line->argv[0], (char * const *)line->argv);
  } else {
    close(fd[1]);
    read(fd[0], buffer, sizeof(buffer));
  }
  return pid;
}

int processOne(char cmd[]) {
  int fd[2], i, nbytes;
  char * split;
  


  pipe(fd);

  split = strtok(cmd, " ");
  while (split != NULL)
  {
    printf("%s\n",split);
    split = strtok(NULL, " ");
  }


  for (size_t i = 0; i < strlen(cmd); i++) {
    /* code */
  }
  const char *ls[] = { cmd, 0 };
  struct cmd line [] = {{ls}};

  exec(fd, line);
  close (fd [1]);
}

void testSub(char * cmd) {

  int fd[2], nbytes;

  pipe(fd);
  const char *ls[] = { cmd, 0 };
  struct cmd line [] = {{ls}};
  exec(fd, line);
  close (fd [1]);
}

char * readInput() {
  int rc;
  char buffer;
  int i = -1;
  size_t len;
  char input[UCHAR_MAX]; //TODO : free !
  //buffer = getchar();
  /*while(buffer != EOF && buffer != '\n' && i++<UCHAR_MAX) {
    input[i] = buffer;
  }*/
  while((rc = read(0, &buffer, 1)) && i++<UCHAR_MAX-1 && buffer != '\n') {
    input[i] = buffer;
  }
  input[i] = '\0';
  //testSub(input);
  processOne(input);
  //printf("%s\n", input);
  return input;
}

int main(int argc, char const *argv[], char **envp) {
  while (*envp)
      printf("%s\n", *envp++);
  ENV = envp;

  char *input;

  //testSub("ls");

  signal(SIGINT, handler);
  while(run) {
    prompt();
    readInput();
  }

  return 0;
}
