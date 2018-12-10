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

job *head = NULL;

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

int main(int argc, char *argv[]) {
  /* TODO : code */
  init();

  pid_t pid;

  jobs_main(argc, argv);
  pid=fork();
  if(pid==0) {
    eval_main(argc, argv);
  } else {
    exec_main(argc, argv);
  }
  return 0;
}
