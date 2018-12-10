#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "structs.h"

/*
 * Process
 */

process * makeProcess(process * next, char **argv, pid_t pid, char c, char s, char st) {
  process *p = malloc(sizeof(process));
  p->next = next;
  p->argv = argv;
  p->pid = pid;
  p->completed = c;
  p->stopped = s;
  p->status = st;
  return p;
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
}

job * makeJob(job *next, char * command, process *p, pid_t pgid, char s, struct termios tm, int in ,int out, int err) {
  job *j = malloc(sizeof(job));
  if(!j) {
    perror("malloc");
    return NULL;
  }
  j->next = next;
  j->command = command;
  j->head = p;
  j->pgid = pgid;
  j->stopping = s;
  j->tmodes = tm;
  j->in = in;
  j->out = out;
  j->err = err;
  return j;
}

job * findJob(pid_t pgid) {
  job *j;
  for(j=head; j; j= j->next) if(j->pgid == pgid) return j;
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

int addJob(pid_t pgid, char * command, process * p) {
  job *j;
  int i = 1;
  if(head == NULL) {
    j = malloc(sizeof(job));
    j->next = NULL;
    j->pgid = pgid;
    j->command = command;
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
        jn->command = command;
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
  printf("[%d] %s (%d %d:%d:%d)\n", j->pgid,j->command, j->stopping, j->in , j->out , j->err);
}

void traceJob(job *j, char * status) {
  fprintf(stderr, "%ld (%s): %s\n", (long)j->pgid, status, j->command);
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

void freeJob(job *j) {
  free(j->head);
  free(j);
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
  addJob(110, "lol", p);
  printAllJobs();
  addJob(111, "lol2", p);
  printAllJobs();
  removeJob(110);
  printAllJobs();
  removeJob(111);
  printAllJobs();
  addJob(112, "lol3", p);
  printAllJobs();
  removeJob(112);
  printAllJobs();
  return 0;
}
