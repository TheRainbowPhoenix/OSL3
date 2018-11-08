#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/wait.h>

int run = 1;

char **ENV;

/* Temporary confing vars */

int SHOW_CWD = 1;
int SHOW_NAME = 1;
int SHOW_GIT = 1;

struct cmd {
  char **argv;
};

void putstr(char *s) {
  while (*s) {
    write(1, &*s++, 1);
  }
}
void _putchar(char c) {
    write(1, &c, 1);
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
  putstr("[39m[49m\n");
  free(ENV);
  exit(0);
}

void handler(int sig) {
  if(sig == SIGINT) {
    //putstr("\n");
    //terminate();
    signal(SIGINT, handler);
  }
}

void ExecHandler(int sig) {
  if(sig == SIGINT) {
    putstr("\n");
    printf("%d\n", getpid());
    kill(getpid(), SIGKILL);
    signal(SIGINT, handler);
  }
}

int startsWith(char * p, char * s) {
  size_t lp = strlen(p);
  return strlen(s) < lp ? 0 : strncmp(p, s, lp) == 0;
}

char * _getENV(char *var) {
  int i = -1;
  char * rtrn;
  while(ENV[++i]) {
    if(startsWith(var, ENV[i])) {
      char *rtrn = strchr(ENV[i], '=');
      return rtrn+1;
    }
  }
  return (NULL);
}

int checkPath(char * path) {
  return 1;
}

int changeDir(char * path) {
  char *cwd;
  char buffer[UCHAR_MAX];
  cwd = getcwd(buffer, UCHAR_MAX);

  if (!chdir(path)) {
    setenv("OLDPWD", cwd, 1);
    setenv("PWD", path, 1);
    return 1;
  } else {
    putstr("[47m[90m cd [37m[49m ");
    if(access(path, F_OK) == -1) putstr("No such file or directory");
    else if (access(path, R_OK) == -1) putstr("Permission denied");
    else putstr("Not a directory");
    _putchar('\n');
    return 0;
  }
}

/* BUILT IN FUNCTIONS */

char * _DEFINED_FUNCTIONS[] = {"cd", "help", "env", "exit"};

int cd(char **args) {
  char	*home;
  if(!args[1]) {
    return changeDir(_getENV("HOME"));
  }
  if(checkPath(args[1])) {
    changeDir(args[1]);
  }
  return (1);
}

int printenv(void) {
  int i = -1;
  char *e;
  while(ENV[++i]) {
    e = ENV[i];
    putstr("[47m[90m ");
    while(*e) {
      if(*e=='=') putstr(" [37m[49m ");
      else _putchar(*e);
      e++;
    }
    _putchar('\n');
  }
  return 0;
}

int help(void) {
  putstr("[47m[90m ");
  putstr("HELP");
  putstr(" [37m[49m ");
  putstr("Currently defined functions:");
  putstr("\n");
  for (size_t i = 0; i < sizeof(*_DEFINED_FUNCTIONS)-1; i++) {
    putstr(" [47m[90m+[37m[49m ");
    putstr(_DEFINED_FUNCTIONS[i]);
    putstr("\n");
    /* code */
  }
  return 0;
}

/* ENDS HERE */

int notBuiltIn(struct cmd *line) {
  if(strcmp(line->argv[0], "exit") == 0) return -1;
  else if(strcmp(line->argv[0], "env") == 0) return (printenv());
  else if(strcmp(line->argv[0], "help") == 0) return (help());
  else if(strcmp(line->argv[0], "cd") == 0) return (cd(line->argv));
  return 1;
}

int exec(int fd[2], struct cmd *line) {
  pid_t pid;
  char buffer[UCHAR_MAX];
  int isBuiltin;

  if((isBuiltin = notBuiltIn(line))<=0) {
    return isBuiltin;
  }

  if((pid=fork()) == -1) return -1;
  if(pid == 0) {
    close(fd[0]);
    //signal(SIGINT, ExecHandler);
    return execvp(line->argv[0], (char * const *)line->argv);
  } else {
    close(fd[1]);
    read(fd[0], buffer, sizeof(buffer));
  }
  wait(&pid);
  return pid;
}

int processOne(char cmd[]) {
  int fd[2], nbytes;
  int i = 0;
  int rtrn = 0;
  char * split;
  char *args[UCHAR_MAX];
  char **com;

  pipe(fd);

  split = strtok(cmd, " ");
  while (split != NULL && i+1<UCHAR_MAX)
  {
    args[i++] = split;
    strncat(split, "\0", sizeof(split)+1);
    //printf("%s\n",split);
    split = strtok(NULL, " ");
  }
  args[i] = 0;

  if(i>=0) {
    if(i==0) {
      com = malloc(2*sizeof(*cmd));
      com[0] = cmd;
      com[1] = 0;
    } else {
      com = malloc(i*sizeof(*cmd));
      size_t n;
      for (n = 0; n < i; n++) {
        com[n] = args[n];
      }
      com[n+1] = 0;
    }
    struct cmd line [] = {{com}};

    rtrn = exec(fd, line);
  }
  close (fd [1]);
  return rtrn;
}

void testSub(char * cmd) {

  int fd[2], nbytes;

  pipe(fd);
  char *ls[] = { cmd, 0 };
  struct cmd line [] = {{ls}};
  exec(fd, line);
  close (fd [1]);
}

int readInput() {
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
  //printf("%s\n", input);
  return processOne(input);
}

int main(int argc, char const *argv[], char **envp) {
  /*while (*envp)
      printf("%s\n", *envp++);*/
  ENV = envp;

  char *input;
  int rtrn;

  //testSub("ls");

  signal(SIGINT, handler);
  while(run) {
    prompt();
    rtrn = readInput();
    if(rtrn<0) run=0;
  }

  return 0;
}
