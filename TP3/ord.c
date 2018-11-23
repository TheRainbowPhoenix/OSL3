/**
 * @Author: uapv1701795
 * @Date:   2018-11-16T17:54:12+01:00
 * @Last modified by:   uapv1701795
 * @Last modified time: 2018-11-23T17:58:37+01:00
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

int run = 1;

typedef struct proc {
  struct proc *next;
  struct proc *prev;
  pid_t pid;
} proc;

proc * current = NULL;

int isBusyWith(pid_t pid, int status) {
  if(pid==0 || errno==ECHILD) return -1;
  if(WIFSTOPPED(status) || WIFEXITED(status) || WIFSIGNALED(status)) return 0;
  return 1;
}

int indexOfProc(pid_t pid) {
  if (current == NULL) return 0;
  proc *p;
  int i=1;
  p=current;
  do
   {
     if(p->pid == pid) return i;
     p=p->next;
   }
  while (p!=NULL && p->pid != current->pid && i++);
  return 0;
}

/*pid_t pidOfIndex(int i) {
  if (current == NULL) return 0;
  proc *p;
  do
   {
     if(i == 0) return p->pid;
     p=p->next;
   }
  while (p!=NULL && p->pid != current->pid && i--);
  return p;
}*/

void step() {
  if(current==NULL) return;
  if(kill(current->pid, SIGSTOP) < 0) perror("SIGSTOP error cannot continue");
  current = current->next;
  if(kill(current->pid, SIGCONT) < 0) perror("SIGCONT error cannot continue");
}

void printProcs() {
  if (current == NULL) return;
  proc *p;
  p=current;
  do
   {
     printf("pid: %d\n", p->pid);
     p=p->next;
   }
  while (p!=NULL && p->pid != current->pid);
}

int addProc(pid_t pid) {
  if (current != NULL && indexOfProc(pid)!=0) return 0;
  proc *b;
  int i = 1;
  if(current == NULL) {
    b = malloc(sizeof(proc));
    b->pid = pid;
    b->prev = b;
    b->next = b;
    current = b;
    return 1;
  } else {
    b = malloc(sizeof(proc));
    b->pid = pid;
    if(current->next == current) current->next = b;
    current->prev->next=b;
    b->prev = current->prev;
    b->next = current;
    current->prev = b;
  }
  return 1;
}

int delProc(pid_t pid) {
	if (current == NULL || indexOfProc(pid)<=0) return 0;
	if(current->next==current) {
		free(current);
		current= NULL;
	} else {
	  proc *p = current;
	  do { p=p->next;}
 	  while (p!=NULL && p->pid != pid);
	  if(p!=NULL) {
		  if(current==p) current = p->next;
		  p->next->prev = p->prev;
		  p->prev->next = p->next;
		  free(p);
		  return 1;
		} else {
			return 0;
	   }
	}
	return 1;
}

void write_pid() {
  FILE * fp;
  pid_t pid = getpid();
  fp = fopen("ordo.dat","w+");
  fprintf(fp, "%d\n",pid);
  fclose(fp);
}

pid_t getLpid() {
	FILE* fp = fopen ("pid.dat", "r");
	pid_t p = -1;
	fscanf(fp, "%d", &p);
	while(!feof(fp)) {
		printf("%d ", p);
		fscanf(fp, "%d", &p);
	}
	return p;
}

void usr_one(int sig) {
  pid_t pid = getLpid();
  printf("Sig 1 %d @ %d\n", sig, pid);
  addProc(pid);
  if(kill(current->pid, SIGSTOP) < 0) perror("SIGSTOP error cannot continue");
  printProcs();
  signal(SIGUSR1, usr_one);
}

void usr_two(int sig) {
  pid_t pid = current->pid;
  printf("Sig 2 %d @ %d\n", sig, pid);
  delProc(pid);
  step();
  printProcs();
  signal(SIGUSR2, usr_two);
}

void test() {
  printf("== %d\n", indexOfProc(125));
  addProc(125);
  addProc(124);
  printProcs();
  printf("== %d\n", current->pid);
  printf("== %d\n", indexOfProc(125));
  addProc(128);
  addProc(129);
  addProc(125);
  printProcs();
  printf("== %d\n", current->pid);
  step();
  printProcs();
  printf("== %d\n", current->pid);
  printf("== %d\n", indexOfProc(125));
  addProc(2267);
  step();
  step();
  step();
  printProcs();
  printf("== %d\n", current->pid);
  printf("== %d\n", indexOfProc(125));
  delProc(2267);
  delProc(125);
  delProc(128);
  delProc(124);
  delProc(129);
  delProc(129);
  printProcs();
}

int main(int argc, char const *argv[]) {
  write_pid();
  signal(SIGUSR1, usr_one);
  signal(SIGUSR2, usr_two);
  while (run) {
    sleep(2);
    if(current!=NULL) {
      step();
      printf("= %d\n", current->pid);
    }
  }
  //if(kill(- j->pid, SIGCONT) < 0) perror("SIGCONT error cannot continue");
  return 0;
}
