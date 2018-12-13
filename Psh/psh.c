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
#include <termios.h>

job *head = NULL;

char * builtin[UCHAR_MAX] = { "cd", "env", "end", "eval", "exit", "help","jobs", "tg", "wait" };
char * ps1 = "$ ";
char * ps2 = "> ";

typedef struct fdef {
  int fd;
  int fw;
} fdef;

fdef *checkOut(char**);

void stdin_set(int cmd) {
    struct termios t;
    tcgetattr(1,&t);
    switch (cmd) {
      case 1: t.c_lflag &= ~ICANON; break;
      default: t.c_lflag |= ICANON; break;
    }
    tcsetattr(1,0,&t);
}

void init() {
  ENV = environment();
  stdin_set(1);
  /* TODO : get from config file */
  // READ ("/etc/profile");
  // READ (".profile");

  ps1 = "[90m>[37m[m ";

  _term = STDIN_FILENO;
  _itty = isatty(_term);

  if(_itty) {
    while(tcgetpgrp(_term) != (_pgid = getpgrp())) kill(-_pgid, SIGTTIN); // Grab the focus

    /* TRAP */

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

/* TODO: move to parsers */

int parseCmd(char raw[]) {
  if(strcmp(raw,"exit")==0) {
    return -1;
  } else {
    return 0;
  }
}

/* TODO : move to input */

/*
 * KEYS
 *
 * 9          : TAB
 * 27 91 65   : up
 * 27 91 66   : down
 * 27 91 67   : ->
 * 27 91 68   : <-
 * 27         : echap
 * 59         : ;
 * 32         : space
 */

char * getPrevious() {
  char p[1024];
  int sz;
  printf("\33[2K\r%s exit", ps1);
  sz = sprintf(p, "exit");
  return p;
}

int readInput() {
	int rc;
	char buffer;
	int i = -1;
	char input[1024]; //TODO : free !
  int cflag = 0;
	//while((rc = read(0, &buffer, 1)) && i++<UCHAR_MAX-1 && buffer != '\n') {
	while((buffer = getchar()) && i++<UCHAR_MAX-1 && buffer != '\n') {
    if(cflag == 2) {
      char *c = getPrevious();
      for (i = 0; i < strlen(c); i++) input[i] = c[i];
      input[i] = '\0';
      cflag = -1;
    }
    if(cflag == 1) {
      if(buffer == 91) cflag = 2;
      else cflag = 0;
    } else if (cflag == 0){
      if(buffer == 27) cflag = 1;
      else input[i] = buffer;
    } else {

    }
	}
	input[i] = '\0';
  printf("%s\n", input);
  return parseCmd(input);

	//return i;
}

void loop(int r) {
  int i;
  size_t len;
  char * line;
  while (r) {
    putstr(ps1);
    i = readInput();
		if(i<0) r=0;
  }
}


/* STUFF */

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
  int run = 1;

  init();

  loop(run);
  /* remove this later */

  char *line = (char*)malloc(sizeof(char)*1024);
  char **elems =(char **)malloc(sizeof(char*)*64);
  while(0) {
    printf("%s",ps1);
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
