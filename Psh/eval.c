/*
 * PSH Eval (fork'n launch)
 */

 #include <signal.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/wait.h>
 #include "structs.h"

void runCmd(process *p, pid_t pgid, int _in, int _out, int _err, int fg) {
  pid_t pid;
  int isBuiltin;

  if((isBuiltin = notBuiltIn(p->argv))<=0) {
  	return;
  }
  if(_itty) {
    pid = getpid();
    if(pgid==0) pgid = pid;
    setpgid(pid, pgid);
    if(fg) tcsetpgrp(_term, pgid);

    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
  }
  if(_in != STDIN_FILENO) {
    dup2(_in, STDIN_FILENO);
    close(_in);
  }
  if(_out != STDOUT_FILENO) {
    dup2(_out, STDOUT_FILENO);
    close(_out);
  }
  if(_err != STDERR_FILENO) {
    dup2(_err, STDERR_FILENO);
    close(_err);
  }
  run(p->argv, ENV);
  printf("Error\n");
}

void runJob(job *j, int fg) {
  process *p;
  pid_t pid;
  int fd[2];
  int _in, _out;

  _in = j->in;
  for(p = j->head; p; p=p->next) {
    if(p->next) {
      if(pipe(fd)<0) {
        trace("eval.c:57 : pipe()", "Broken pipe");
      }
      _out = fd[1];
    } else {
      _out=j->out;
    }
    pid = fork();
    if(pid==0) {
      runCmd(p, j->pgid, _in, _out, j->err, fg);
    } else if(pid<0) {
      trace("eval.c:64 : fork()", "fork failed");
    } else {
      p->pid = pid;
      if(_itty) {
        if(!j->pgid) j->pgid = pid;
        setpgid(pid, j->pgid);
      }
    }
    if(_in != j->in) close(_in);
    if(_out != j->out) close(_out);
    _in = fd[0];
  }
  if(!_itty) waitJob(j);
  else if(fg) jobFg(j, 0);
  else jobBg(j, 0);
}

int eval_main(int argc, char *argv[]) {
  printf("[47m[90m EVAL MAIN TEST [37m[49m\n");
  char** exe = (char *[]){"ls", "-la", NULL};
  char** exe2 = (char *[]){"echo", "OwO","world", "yay", NULL};
  char** exe3 = (char *[]){"cut", "-b-4", NULL};
  process *p = makeProcess(NULL, exe, 10, 0, 0, 0);
  process *p3 = makeProcess(NULL, exe3, 10, 0, 0, 0);
  process *p2 = makeProcess(p3, exe2, 10, 0, 0, 0);
  pid_t pgid = getpid();
  job *j = makeJob(NULL, "echo OWO world yay | cut -b-4", p2, pgid, 0, _tmodes, 0, 1, 2);
  printf("> should write 'OWO' :\n");
  runJob(j, 1);
  //runCmd(p2, pgid, 0, 1, 2, 1);
  printf("> should write ls output :\n");
  //runCmd(p, pgid, 0, 1, 2, 1);
  //runCmd(process *p, pid_t pgid, int _in, int _out, int _err, int fg)
  return 0;
}
