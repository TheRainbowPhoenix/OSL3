#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>
#include "structs.h"

/*
 * Process
 */

void dumpProcess(process *p) {
  for (size_t i = 0; p->argv[i]; i++) {
    printf("%s ", p->argv[i]);
  }
}

process * makeProcess(process * next, char **argv, pid_t pid, char c, char s, char st) {
  process *p = malloc(sizeof(process));
  if(!p) {
    perror("malloc");
    return NULL;
  }
  p->next = next;
  p->argv = argv;
  p->pid = pid;
  p->completed = c;
  p->stopped = s;
  p->status = st;
  return p;
}

process * makeEmptyProcess() {
  process *p = malloc(sizeof(process));
  if(!p) {
    perror("malloc");
    return NULL;
  }
  p->next = NULL;
  p->argv = NULL;
  p->completed = 0;
  p->stopped = 0;
  return p;
}

void freeProcess(process *p) {
  if(!p->argv) return;
  int i;
  for (i = 0; p->argv[i] && i<UCHAR_MAX; i++) {
    free(p->argv[i]);
  }
  free(p->argv[i]);
}

/*
 * Struct utils
 */

job* makeEmptyJob() {
  job *j = malloc(sizeof(job));
  if(!j) {
    perror("malloc");
    return NULL;
  }
  j->head = NULL;
  j->valid = 1;
  j->id = 0;
  j->infile = NULL;
  j->outfile = NULL;
  j->in = STDIN_FILENO;
  j->out = STDOUT_FILENO;
  j->err = STDERR_FILENO;
  j->fg = 1;
  j->wmode = 0;
  j->stopping = 0;
  j->pgid = 0;
  return j;
}

job * makeJob(job *next, process *p, pid_t pgid, char s, struct termios tm, int in ,int out, int err) {
  job *j = malloc(sizeof(job));
  if(!j) {
    perror("malloc");
    return NULL;
  }
  j->next = next;
  j->head = p;
  j->pgid = pgid;
  j->stopping = s;
  j->tmodes = tm;
  j->in = in;
  j->out = out;
  j->err = err;
  j->wmode = 0;
  return j;
}


void freeJob(job *j) {
  if(!j) return;
  process *p = j->head;
  while(p) {
    process *p2 = p->next;
    freeProcess(p);
    p = p2;
  }
  free(j->infile);
  free(j->outfile);
}

job * findJob(pid_t pgid) {
  job *j;
  for(j=head; j; j= j->next) if(j->pgid == pgid) return j;
  return NULL;
}

job * findJobId(int id) {
  if(id<1) return NULL;
  job *j;
  for(j=head; j; j= j->next) if(j->id == id) return j;
  return NULL;
}

int isJobStopped(job *j) {
  process *p;
  for(p=j->head; p; p=p->next) if(!p->completed && !p->stopped) return 0;
  return 1;
}

int isJobCompleted(job *j) {
  process *p;
  for(p=j->head; p; p=p->next) if(!p->completed) return 0;
  return 1;
}

void dumpJob(job *j) {
  if(!j->id) {
    fprintf(stderr, "Invalid job\n");
    return;
  }
  process *p;
  int i = 0;
  if(j->infile) printf("In : %s\n", j->infile);
  if(j->outfile) printf("Out : %s\n", j->outfile);
  for (p = j->head; p; p=p->next) {
    dumpProcess(p);
  }
}

int addJob(pid_t pgid, process * p) {
  job *j;
  int i = 1;
  if(head == NULL) {
    j = malloc(sizeof(job));
    j->next = NULL;
    j->pgid = pgid;
    //j->command = command;
    j->head = p;
    j->stopping = 0;
    j->tmodes = _tmodes;
    j->in = 0;
    j->out = 1;
    j->err = 2;
    head = j;
    return 1;
  } else {
    for(j=head; j!=NULL; j=j->next, i++) {
      if(j->next == NULL) {
        job *jn;
        jn = malloc(sizeof(job));
        jn->next = NULL;
        jn->pgid = pgid;
        //jn->command = command;
        jn->head = p;
        jn->stopping = 0;
        jn->tmodes = _tmodes;
        jn->in = 0;
        jn->out = 1;
        jn->err = 2;
        j->next = jn;
        return i+1;
      }
    }
  }
  return 0;
}

void printJob(job *j) {
  printf("[%d] (%d %d:%d:%d)\n", j->pgid, j->stopping, j->in , j->out , j->err);
}

void traceJob(job *j, char * status) {
  fprintf(stderr, "%ld (%s)\n", (long)j->pgid, status);
}

void printAllJobs() {
  job *j;
  if(head == NULL) printf("No jobs !\n");
  else for(j=head; j; j= j->next) printJob(j);
}

int removeJob(pid_t pgid) {
  job *j;
  if(head == NULL) return 0;
  //killJob(pid);
  if(head->next == NULL && head->pgid == pgid) {
    free(head);
    head = NULL;
    return 1;
  }
  if(head->pgid == pgid && head->next != NULL) {
		free(head->head);
		head = head->next;
		return 1;
	}
  for(j=head; j!=NULL && j->next!=NULL; j=j->next) {
    if(j->next->pgid == pgid) {
			free(j->next->head);
			free(j->next);
			//free(j->next->command);
			j->next = j->next->next;
			return 1;
		}
  }
  return 0;
}

int processStatus(pid_t pid, int status) {
  job *j;
  process *p;
  if(pid > 0) {
    for (j = head; j ; j=j->next) {
      for (p = j->head; p; p=p->next) {
        if(p->pid == pid) {
          p->status = status;
          if(WIFSTOPPED(status)) p->stopped = 1;
          else {
            p->completed = 1;
            if(WIFSIGNALED(status)) fprintf(stderr, "%d: Terminated by signal %d.\n", pid, WTERMSIG(p->status));
          }
          return 0;
        }
      }
    }
    return -1;
  } else if (pid == 0 || errno == ECHILD) {
    return -1;
  } else {
    perror("waitpid (processStatus)");
    return -1;
  }
}

void updateJobs() {
  int status;
  pid_t pid;
  do {
    pid = waitpid(WAIT_ANY, &status, WUNTRACED|WNOHANG);
  } while(!processStatus(pid, status));
}

void waitJob(job *j) {
  int status;
  pid_t pid;
  do {
    pid = waitpid(WAIT_ANY, &status, WUNTRACED);
  } while(!processStatus(pid, status) && isJobStopped(j) && isJobCompleted(j));
}

void notifyJobs() {
  job *j, *je, *jn;
  process *p;

  updateJobs();
  je = NULL;
  for(j=head; j; j = jn) {
    jn = j->next;
    if(isJobCompleted(j)) {
      traceJob(j, "completed");
      if(je) je->next = jn;
      else head = jn;
      freeJob(j);
    } else if (isJobStopped(j) && !j->stopping) {
      traceJob(j, "stopped");
      j->stopping = 1;
      je = j;
    } else {
      je = j;
    }

  }
}

void jobFg(job *j, int cnt) {
  j->fg = 1;
  tcsetpgrp(_term, j->pgid);
  if(cnt) {
    tcsetattr(_term, TCSADRAIN, &j->tmodes);
    if(kill(- j->pgid, SIGCONT) < 0) perror("SIGCONT : cannot continue");
  }
  waitJob(j);
  tcsetpgrp(_term, _pgid);
  tcgetattr(_term, &j->tmodes);
  tcsetattr(_term, TCSADRAIN, &_tmodes);
}

void jobBg(job *j, int cnt) {
  j->fg = 0;
  if(cnt) {
    if(kill(- j->pgid, SIGCONT) < 0) perror("SIGCONT : cannot continue");
  }
}

void setJobRunning(job *j) {
  process *p;
  for(p=j->head; p; p=p->next) p->stopped = 0;
  j->stopping = 0;
}

void continueJob(job *j, int fg) {
  setJobRunning(j);
  if(fg) jobFg(j, 1);
  else jobBg(j, 1);
}

/*
 * Exec-working fuctions
 */


int forkShell(job *j, int mode) {
  pid_t pid;
  if((pid=fork()) == -1) trace("fork()","Fork error");
  if(pid==0) {
    //forkChild();
  } else {
    //forkParent();
  }
  return pid;
}

/*
 * NotUnitTest (tm) Striked again !
 */

int jobs_main(int argc, char *argv[]) {
  printf("[47m[90m JOBS MAIN TEST [37m[49m\n");
  //(process * next, char **argv, pid_t pid, char c, char s, char st)
  process *p = makeProcess(NULL, argv, 10, 0, 0, 0);
  addJob(110, p);
  printAllJobs();
  addJob(111, p);
  printAllJobs();
  removeJob(110);
  printAllJobs();
  removeJob(111);
  printAllJobs();
  addJob(112, p);
  printAllJobs();
  removeJob(112);
  printAllJobs();
  return 0;
}
