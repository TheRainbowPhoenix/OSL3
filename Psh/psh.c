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
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>

job *head = NULL;

char * builtin[UCHAR_MAX] = { "cd", "env", "end", "eval", "exit", "help","jobs", "tg", "wait" };
char * ps1 = "$ ";
char * ps2 = "> ";

char * prev = "exit";

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

int main(int argc, char *argv[]) {
  int run = 1;

  init();

  loop(run); //YES IT PARSES LIKES *** => ALL THE TESTS ARE HERE AND WORKING

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
