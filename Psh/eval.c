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

void runBuiltin(process *p, int _in, int _out, int _err) {
    if(strcmp(p->argv[0], "cd") == 0) {
      cd(p->argv);
    }
}

void runJob(job *j, int fg, int *id) {
  process *p;
  pid_t pid;
  int fd[2];
  int _in, _out;

  if(j->infile) {
    j->in = open(j->infile, O_RDONLY);
    if(j->in < 0) {
      trace(j->infile,"open error");
    }
  }
  if(j->outfile) {
    j->out = open(j->outfile, O_RDWR|O_CREAT|O_TRUNC, 0666);
    if(j->out < 0) {
      trace(j->outfile,"open error");
    }
  }
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
    if(isBuiltin(p)) {
      runBuiltin(p, _in, _out, j->err);
      p->completed = 1;
    } else {
      pid = fork();
      if(pid==0) {
        runCmd(p, j->pgid, _in, _out, j->err, fg);
      } else if(pid<0) {
        trace("eval.c:64 : fork()", "fork failed");
      } else {
        p->pid = pid;
        if(_itty) {
          if(!j->pgid) {
            j->pgid = pid;
            j->id = *id;
            *id = *id+1;
          }
          setpgid(pid, j->pgid);
        }
      }
    }
    if(_in != j->in) close(_in);
    if(_out != j->out) close(_out);
    _in = fd[0];
  }
  if(!_itty) waitJob(j);
  else if(fg) jobFg(j, 0);
  else {
    jobBg(j, 0);
    traceJob(j, "background");
  }
}


int eval_main(int argc, char *argv[]) {
  printf("[47m[90m EVAL MAIN TEST [37m[49m\n");
  char** exe = (char *[]){"ls", "-la", NULL};
  char** exe2 = (char *[]){"echo", "OWO","world", "yay", NULL};
  char** exe3 = (char *[]){"cut", "-b-4", NULL};
  char** exe4 = (char *[]){"tr", "'W'", "'w'", NULL};
  char** exe5 = (char *[]){"awk", "{print $1\" what is this\"}", NULL};
  char** exe6 = (char *[]){"sed", "-e", "s/this/dis/g", NULL};
  char** exe7 = (char *[]){"xargs", "-I", "{}", "echo", "{}", "?", NULL};
  char** exee = (char *[]){"cat", NULL};
  char** exee2 = (char *[]){"sed","-e","s/very/not that/g", NULL};
  char** exee3 = (char *[]){"cut","-b-20", NULL};
  char** exee4 = (char *[]){"tr","[:lower:]","[:upper:]", NULL};


  process *p = makeProcess(NULL, exe, 10, 0, 0, 0);
  process *p3 = makeProcess(NULL, exe3, 10, 0, 0, 0);
  process *p2 = makeProcess(p3, exe2, 10, 0, 0, 0);

  process *P7 = makeProcess(NULL, exe7, 10, 0, 0, 0);
  process *P6 = makeProcess(P7, exe6, 10, 0, 0, 0);
  process *P5 = makeProcess(P6, exe5, 10, 0, 0, 0);
  process *P4 = makeProcess(P5, exe4, 10, 0, 0, 0);
  process *P3 = makeProcess(P4, exe3, 10, 0, 0, 0);
  process *P2 = makeProcess(P3, exe2, 10, 0, 0, 0);
  process *pe = makeProcess(NULL, exee, 10, 0, 0, 0);
  process *Pe4 = makeProcess(NULL, exee4, 10, 0, 0, 0);
  process *Pe3 = makeProcess(Pe4, exee3, 10, 0, 0, 0);
  process *Pe2 = makeProcess(Pe3, exee2, 10, 0, 0, 0);
  process *Pe = makeProcess(Pe2, exee, 10, 0, 0, 0);

  pid_t pgid = getpid();
  job *j = makeJob(NULL, p2, pgid, 0, _tmodes, 0, 1, 2);
  //echo OWO world yay | cut -b-4 | tr 'W' 'w' | awk '{print $1" what is this"}' | sed 's/this/dis/g' | xargs -I {} echo {} "?"
//  printf("> should write 'OWO' :\n");
  int id = 1;
  runJob(j, 1, &id);

  job *j2 = makeJob(NULL, P2, pgid, 0, _tmodes, 0, 1, 2);
  j2->outfile = "f1.txt";
  runJob(j2, 1, &id);


  job *j3 = makeJob(NULL, pe, pgid, 0, _tmodes, 0, 1, 2);
  j3->infile = "in";
  runJob(j3, 1, &id);

  //cat <in| sed -e 's/very/not that/g' | cut -b-20 | tr [:lower:] [:upper:] >out
  job *j4 = makeJob(NULL, Pe, pgid, 0, _tmodes, 0, 1, 2);
  j4->infile = "in";
  j4->outfile = "out";
  runJob(j4, 1, &id);
  //runCmd(p, pgid, 0, 1, 2, 1);
  //printf("> should write ls output :\n");
  //wait(NULL);
  //runCmd(p, pgid, 0, 1, 2, 1);
  //runCmd(process *p, pid_t pgid, int _in, int _out, int _err, int fg)
  return 0;
}
