/*
 * Phebe's Sassy Helper
 *
 * HEADER !!!
 */

#include "structs.h"
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>

job *head = NULL;

char * builtin[UCHAR_MAX] = { "cd", "env", "end", "eval", "exit", "help","jobs", "tg", "wait" };

typedef struct fdef {
  int fd;
  int fw;
} fdef;

fdef *checkOut(char**);

void init() {
  ENV = environment();

  _term = STDIN_FILENO;
  _itty = isatty(_term);

  if(_itty) {
    while(tcgetpgrp(_term) != (_pgid = getpgrp())) kill(-_pgid, SIGTTIN); // Grab the focus

    signal (SIGINT, SIG_IGN);
    signal (SIGQUIT, SIG_IGN);
    signal (SIGTSTP, SIG_IGN);
    signal (SIGTTIN, SIG_IGN);
    signal (SIGTTOU, SIG_IGN);
    signal (SIGCHLD, SIG_IGN);

    _pgid = getpid();
    if(setpgid(_pgid, _pgid) <0) {
      trace("psh.c:23 : setpgid()","Could not create process group");
    }
    tcsetpgrp(_term, _pgid);
  	tcgetattr(_term, &_tmodes);
  }
}

int notBuiltIn(char **argv) {
	if(strcmp(argv[0], "exit") == 0) return -1;
	return 1;
}

fdef *checkOut(char** elems) {
  int i = 0;
  fdef *fd = malloc(sizeof(fdef));
  while(elems[i] !=NULL) {
    if(strcmp(elems[i],">")==0) {
      fd->fd=open(elems[i+1],O_CREAT|O_TRUNC|O_WRONLY,0644);
      if(fd->fd<0) perror("open");
      fd->fw=STDOUT_FILENO;
      elems[i]=NULL;
      return fd;
    }
    if(strcmp(elems[i],"<")==0) {
      fd->fd=open(elems[i+1],O_CREAT|O_TRUNC|O_WRONLY,0644);
      if(fd->fd<0) perror("open");
      fd->fw=STDIN_FILENO;
      elems[i]=NULL;
      return fd;
    }
    i++;
  }
  return NULL;
}

void readline(char *line)
{
    fgets(line,1024,stdin);
    line[strcspn(line,"\n")]='\0';
}

void parseline(char *args,char **tokens)
{
  int i=0;
  tokens[0]=strtok(args," ");
  if(tokens[0]==NULL)
    return;
  do {
    if(i>=(sizeof(tokens)/sizeof(char*))-1)
      tokens=realloc(tokens,sizeof(char*)*((sizeof(tokens)/sizeof(char*)+64)));
    tokens[++i]=strtok(NULL," ");
  } while(tokens[i]!=NULL);

}

void waitfor(pid_t pid) {
  int status;
  tcsetpgrp(_term, pid);
  waitpid(pid, &status, WUNTRACED);
  if(WIFSTOPPED(status)) {
    // stuff
  }
  tcsetpgrp(_term, getpid());
}

void _exec(char **elems, fdef *fd) {
  pid_t pid = fork();
  if(pid==0) {
    signal (SIGINT, SIG_DFL);
    signal (SIGQUIT, SIG_DFL);
    signal (SIGTSTP, SIG_DFL);
    signal (SIGTTIN, SIG_DFL);
    signal (SIGTTOU, SIG_DFL);
    signal (SIGCHLD, SIG_DFL);
    setpgid(0,0);
    if(fd !=NULL) {
      dup2(fd->fd,fd->fw);
      close(fd->fd);
    }
    run(elems, ENV);
  } else if (pid<0) {
    fprintf(stderr, "Fork error\n");
  } else {
    setpgid(pid,pid);
    waitfor(pid);
    if(fd !=NULL) {
      free(fd);
    }
  }
}

int shellexec(char **elems) {
  fdef *fd = checkOut(elems);
  if(elems[0] == NULL) return 1;
  if(strcmp(elems[0],"exit")==0) {
    return 0;
  } else {
    _exec(elems, fd);
  }
  return 1;
}

int main(int argc, char *argv[]) {
  /* TODO : code */
  init();

  char *line = (char*)malloc(sizeof(char)*1024);
  char **elems =(char **)malloc(sizeof(char*)*64);
  int run = 0;
  while(run) {
    printf(">");
    readline(line);
    parseline(line, elems);
    if(elems[0]==NULL) {
      run = 0;
    }
    run = shellexec(elems);
  }
  free(line);
  free(elems);

  /* Test stuff */

  pid_t pid;

  jobs_main(argc, argv);
  builtin_main(argc, argv);
  pid=fork();
  if(pid==0) {
    eval_main(argc, argv);
  } else {
    wait(NULL);
    exec_main(argc, argv);
  }
  return 0;
}
