#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <termios.h>

#define WAIT_ANY (-1)

int run = 1;

char **ENV;

pid_t _pgid;
struct termios _tmodes;

int _term;
int _itty;

int putErr(char * err);

/* Temporary confing vars */

int SHOW_CWD = 1;
int SHOW_NAME = 1;
int SHOW_GIT = 0;

char * _DEFINED_MODULES[] = {"CWD", "NAME", "GIT",0};

/* Modules functions */

int isModuleActive(char * module) {
  if(strcmp(module, _DEFINED_MODULES[0]) == 0) return SHOW_CWD;
  else if(strcmp(module, _DEFINED_MODULES[1]) == 0) return SHOW_NAME;
  else if(strcmp(module, _DEFINED_MODULES[2]) == 0) return SHOW_GIT;
  else return -1;
}

void toggleModule(char * module) {
  if(strcmp(module, _DEFINED_MODULES[0]) == 0) SHOW_CWD=!SHOW_CWD;
  else if(strcmp(module, _DEFINED_MODULES[1]) == 0) SHOW_NAME=!SHOW_NAME;
  else if(strcmp(module, _DEFINED_MODULES[2]) == 0) SHOW_GIT=!SHOW_GIT;
  else putErr("Module not found");
}

/* Ends here */

struct cmd {
  char **argv;
};

typedef struct bgJob {
  struct bgJob *next;
  pid_t pid;
  char * line;
} bgJob;

bgJob * bgJobs = NULL;

/* bgJob functions */


int killJob(pid_t pid) {
  if(getpgid(pid) < 0) return 0;
  kill(pid, SIGINT);
  return 1;
}

int removeBgJob(pid_t pid) {
  bgJob *b;
  if(bgJobs == NULL) return 0;
  killJob(pid);
  if(bgJobs->next == NULL && bgJobs->pid == pid) {
    free(bgJobs->line);
    bgJobs = NULL;
    return 1;
  }
  if(bgJobs->pid == pid && bgJobs->next != NULL) {
    free(bgJobs->line);
    bgJobs = bgJobs->next;
    return 1;
  }
  for(b=bgJobs; b!=NULL && b->next!=NULL; b=b->next) {
    if(b->next->pid == pid) {
      free(b->next->line);
      b->next = b->next->next;
      return 1;
    }
  }
  return 0;
}

void updateJobs() {
  bgJob *b;
  for(b=bgJobs; b!=NULL; b=b->next) {
    if(getpgid(b->pid) < 0) removeBgJob(b->pid);
  }
}

void killAllJobs() {
  bgJob *b;
  int i = 1;
  for(b=bgJobs; b!=NULL; b=b->next, i++) {
    killJob(b->pid);
    printf("[%d] : Stopped\n", i);
  }
  updateJobs();
}

/* Ends here */

int notBuiltIn(char **argv);

void putstr(char *s) {
  while (*s) {
    write(1, &*s++, 1);
  }
}
void _putchar(char c) {
    write(1, &c, 1);
}

void terminate() {
  killAllJobs();
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
    /*printf("%d\n", getpid());*/
    kill(getpid(), SIGKILL);
    signal(SIGINT, handler);
  }
}

void pass(int sig) {}

void chldHandler(int sig) {
	pid_t pid;
	int status;

	pid = waitpid(WAIT_ANY, &status, WNOHANG | WUNTRACED);
	if (pid == -1) {
		perror("Wait error");
		//exit(1);
	}
	if (pid > 0) {

		//printf("%d %d %d\n", pid,getpgrp(),  getpid());

		if(WIFEXITED(status)) {
			printf("[%d] Done (Status: %d)\n", pid, WEXITSTATUS(status));
      removeBgJob(pid);
			return;
		}

		if(WIFSIGNALED(status)) {
			printf("[%d] Done (Signal: %d)\n", pid, WTERMSIG(status));
			return;
		}
		if(WIFSTOPPED(status)) {
			printf("[%d] Stopped (Signal: %d)\n", pid, WSTOPSIG(status));
			return;
		}
	}
}

int	ft_atoi(char *str)
{
	int rslt;
	int sgn;

	rslt = 0;
	sgn = 1;
	while (*str == ' ' || (*str >= 9 && *str <= 13))
		str++;
	if (*str == '-')
		sgn = -1;
	if (*str == '-' || *str == '+')
		str++;
	while (*str >= '0' && *str <= '9')
	{
		rslt = rslt * 10 + *str - '0';
		str++;
	}
	return (sgn * rslt);
}

char* ft_itoa(int i, char buff[], int base) {
  char const digit[] = "0123456789";
  char *rslt = buff;
  if(i<0) {
    *rslt++ = '-';
    i *= -1;
  }
  int nb = i;
  while(nb) {
    ++rslt;
    nb /= base;
  }
  *rslt = '\0';
  while(i) {
    *--rslt = digit[i%base];
    i /= base;
  }
  return rslt;

}

int startsWith(char * p, char * s) {
  size_t lp = strlen(p);
  return strlen(s) < lp ? 0 : strncmp(p, s, lp) == 0;
}

int isDir(char * path) {
  struct stat stats;
  if (stat(path, &stats)!= 0) return 0;
  return S_ISDIR(stats.st_mode);
}

int hasDir(char *path, char *dname) {
  DIR *d;
  struct dirent *dir;
  d = opendir(path);
  if (d) {
    while((dir = readdir(d)) != NULL) if(strcmp(dir->d_name, dname) == 0) return 1;
  }
  return 0;
}

int isGitDir(char *path) {
  FILE * fp;
  char out[UCHAR_MAX];
  fp = popen("git rev-parse --git-dir 2> /dev/null", "r");
  if(fp == NULL) return 0;
  while (fgets(out, sizeof(out)-1, fp) != NULL) {
    if(startsWith("/",out) || startsWith(".git",out)) return 1;
  }
  return 0;
}

char * getGitBranch(char *path) {
  FILE * fp;
  char out[UCHAR_MAX];
  char *rtrn;
  fp = popen("git branch | grep \\* | cut -d ' ' -f2 2> /dev/null", "r");
  if(fp == NULL) return 0;
  while (fgets(out, sizeof(out)-1, fp) != NULL) {
  }
  size_t i;
  rtrn = (char *) malloc(sizeof *out);
  for (i = 0; out[i]!='\n' && out[i]!='\0'; i++) {
    rtrn[i] = out [i];
  }
  rtrn[i+1] = '\0';
  //strncpy(out, rtrn, sizeof(out));
  return rtrn;
}

int getGitPushPending(char *path) {
  FILE * fp;
  char out[UCHAR_MAX];
  char *rtrn;
  int cnt = 0;
  fp = popen("git cherry -v 2> /dev/null", "r");
  if(fp == NULL) return 0;
  while (fgets(out, sizeof(out)-1, fp) != NULL) {
    if(startsWith("+ ", out)) cnt++;
  }
  //strncpy(out, rtrn, sizeof(out));
  return cnt;
}

int getGitAdded(char *path) {
  FILE * fp;
  char out[UCHAR_MAX];
  char *rtrn;
  int cnt = 0;
  fp = popen("git status --porcelain --branch 2> /dev/null", "r");
  if(fp == NULL) return 0;
  while (fgets(out, sizeof(out)-1, fp) != NULL)  if(startsWith("A  ", out)) cnt++;
  return cnt;
}

int getGitModified(char *path) {
  FILE * fp;
  char out[UCHAR_MAX];
  char *rtrn;
  int cnt = 0;
  fp = popen("git status --porcelain --branch 2> /dev/null", "r");
  if(fp == NULL) return 0;
  while (fgets(out, sizeof(out)-1, fp) != NULL)  if(startsWith(" M ", out)) cnt++;
  return cnt;
}
int getGitDeleted(char *path) {
  FILE * fp;
  char out[UCHAR_MAX];
  char *rtrn;
  int cnt = 0;
  fp = popen("git status --porcelain --branch 2> /dev/null", "r");
  if(fp == NULL) return 0;
  while (fgets(out, sizeof(out)-1, fp) != NULL) if(startsWith(" D ", out)) cnt++;
  return cnt;
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
    return 0;
  } else {
    putstr("[47m[90m cd [37m[49m ");
    if(access(path, F_OK) == -1) putstr("No such file or directory");
    else if (access(path, R_OK) == -1) putstr("Permission denied");
    else putstr("Not a directory");
    _putchar('\n');
    return 1;
  }
}

int runCmd(struct cmd *line, pid_t pgid, int _in, int _out, int fg) {
  pid_t pid;
  int isBuiltin;

  if((isBuiltin = notBuiltIn(line->argv))<=0) {
    return isBuiltin;
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

  return execvp(line->argv[0], (char * const *)line->argv);
  perror("Exec format error");
  exit(1);
}

int bgCmd(char **args) {
  if(!args[1] || strcmp(args[1], "--help") == 0) {
    putstr("[47m[90m ");
    putstr("bg");
    putstr(" [37m[49m ");
    putstr("<command> Execute in backgound");
    _putchar('\n');
    return 0;
  }
  //runProcess(process *p, pid_t pgid, int _in, int _out, int _err, int fg)
  //runJjob(Jjob *j, 0)
  return 0;
}

/*
typedef struct job {
  struct job *next;
  char * last;
  struct cmd * command;
  pid_t pgid;
  char completed;
  char stopped;
  int status;
} job;

 */

/*void bgJob(job *j, int c) {
  if(c) {
    if(kill(- j->pgid, SIGCONT) < 0) perror("SIGCONT error cannot continue");
  }
}*/

int isBusyWith(pid_t pid, int status) {
  if(pid==0 || errno==ECHILD) return -1;
  if(WIFSTOPPED(status) || WIFEXITED(status) || WIFSIGNALED(status)) return 0;
  return 1;
}

int indexOfBgJob(pid_t pid) {
  bgJob *b;
  int i=1;
  for(b=bgJobs; b!=NULL; b=b->next, i++) {
    if(b->pid == pid) return i;
  }
  return 0;
}

pid_t pidOfBgJob(int i) {
  bgJob *b;
  pid_t p=-1;
  for(b=bgJobs; b!=NULL; b=b->next, i--) {
    if(i==1 && b!=NULL) return b->pid;
  }
  return p;
}

int addBgJob(pid_t pid, char * line) {
  bgJob *b;
  int i = 1;
  if(bgJobs == NULL) {
    b = malloc(sizeof(bgJob));
    b->next = NULL;
    b->pid = pid;
    b->line = line;
    bgJobs = b;
    return 1;
  } else {
    for(b=bgJobs; b!=NULL; b=b->next, i++) {
      if(b->next == NULL) {
        bgJob *jb;
        jb = malloc(sizeof(bgJob));
        jb->next = NULL;
        jb->pid = pid;
        jb->line = line;
        b->next = jb;
        return i+1;
      }
    }
  }
  return 0;
}

int exec(int fd[2], struct cmd *line, int bg) {
  pid_t pid;
  char buffer[UCHAR_MAX];
  int isBuiltin;

  if((isBuiltin = notBuiltIn(line->argv))<=0) {
    return isBuiltin;
  }
  /*
  void fgJjob(Jjob *j, int c) {
    tcsetpgrp(_term, j->pgid);
    if(c) {
      tcsetattr(_term, TCSADRAIN, &j->tmodes);
      if(kill(- j->pgid, SIGCONT) < 0) perror("SIGCONT error cannot continue");
    }
    waitJjob(j);
    tcsetpgrp(_term, _pgid);
    tcgetattr(_term, &j->tmodes);
    tcsetattr(_term, TCSADRAIN, &_tmodes);
  }

  void bgJjob(Jjob *j, int c) {
    if(c) {
      if(kill(- j->pgid, SIGCONT) < 0) perror("SIGCONT error cannot continue");

    }
  }

   */
  if (bg == 1) {
 	 signal(SIGCHLD, chldHandler);
  } else {
	 signal(SIGCHLD, SIG_DFL);
 }

  if((pid=fork()) == -1) return -1;
  if(pid == 0) {
    close(fd[0]);
    if(bg) {
      //close(STDOUT_FILENO);
      int devnull = open("/dev/null",O_WRONLY | O_CREAT, 0666);
      dup2(devnull, STDIN_FILENO);
      dup2(devnull, STDOUT_FILENO);

    } else {
      dup2(fd[0], STDIN_FILENO);
      dup2(fd[0], STDOUT_FILENO);
    }
    //signal(SIGINT, ExecHandler);
    return execvp(line->argv[0], (char * const *)line->argv);
  } else {
    close(fd[1]);
    if(!bg) read(fd[0], buffer, sizeof(buffer));
    else {
      int ind = addBgJob(pid, line->argv[0]);
      printf("[%d] %d \t%s\n", ind, pid, line->argv[0]);
    }
  }
  //if(bg == 0) wait(&pid);
  if(bg == 0) {
    pid_t cpid;
    int status;
    tcsetpgrp(_term, _pgid);
    do {
      cpid=waitpid(WAIT_ANY, &status, WUNTRACED);
    } while(isBusyWith(pid, status));
    tcsetattr(_term, TCSADRAIN, &_tmodes);
  } else {
    //if(kill(- pid, SIGCONT) < 0) perror("SIGCONT error cannot continue");
  }
  return pid;
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
    if (getcwd(cwd, UCHAR_MAX) != NULL) {
      if(isGitDir(cwd)) {
        /*
        ≡ No changes
        ↓ commit waiting to pull
        ↑ commit waiting to push
        */
        int pending = getGitPushPending(cwd);
        int modified = getGitModified(cwd);
        int added = getGitAdded(cwd);
        int deleted = getGitDeleted(cwd);
        char pbuff[UCHAR_MAX];
        char cbuff[UCHAR_MAX];
        if(pending>0) {
          sprintf(pbuff, " ↑%d", pending);
        } else {
          sprintf(pbuff, " ≡");
        }
        if(modified != 0) {
          if(added != 0) {
              if(deleted != 0) {
                sprintf(cbuff, " ~%d +%d -%d", modified, added, deleted);
              } else {
                sprintf(cbuff, " ~%d +%d", modified, added);
              }
          } else {
            if(deleted != 0) {
              sprintf(cbuff, " ~%d -%d", modified, deleted);
            } else {
              sprintf(cbuff, " ~%d", modified);
            }
          }
        } else {
          if(added != 0) {
            sprintf(cbuff, " +%d", added);
            sprintf(cbuff, " +%d -%d", added, deleted);
          } else {
            if(deleted != 0) {
              sprintf(cbuff, " -%d", deleted);
            } else {
              sprintf(cbuff, " ");
            }
          }
        }
        //sprintf(cbuff, " ~%d +%d -%d", modified, added, deleted);
        printf("[47m[90m%s%s%s%s%s[37m[49m ", "[", getGitBranch(cwd),pbuff, cbuff, "]");

      }
    }
  }
  printf("\n");
  putstr("[90m>[37m[m "); /* TODO: Prompt custom */
}


int putErr(char * err) {
  putstr(" [100m[91m");
  putstr(" ⚠  [47m[90m ");
  putstr(err);
  putstr(" [37m[49m ");
  _putchar('\n');
  return 0;
}

/* BUILT IN FUNCTIONS */

char * _DEFINED_FUNCTIONS[] = {"cd", "env", "end", "eval", "exit", "help","jobs", "tg", "wait",0};

int cd(char **args) {
  char	*home;
  if(!args[1]) {
    changeDir(_getENV("HOME"));
    return 0;
  }
  if(checkPath(args[1])) {
    return changeDir(args[1]);
  }
  return 1;
}

int tg(char **args) {
  if(!args[1] || strcmp(args[1], "--help") == 0) {
    putstr("[47m[90m ");
    putstr("tg");
    putstr(" [37m[49m ");
    putstr("<entry> Toggle shell modules. Try --list to display them.");
    _putchar('\n');
  } else if (strcmp(args[1], "-l") == 0 || strcmp(args[1], "--list") == 0 ) {
    for (size_t i = 0; _DEFINED_MODULES[i]; i++) {
      if(isModuleActive(_DEFINED_MODULES[i])) putstr(" [47m[90m+[37m[49m ");
      else putstr(" [47m[90m-[37m[49m ");
      putstr(_DEFINED_MODULES[i]);
      putstr("\n");
      /* code */
    }
  } else if (isModuleActive(args[1])>=0) {
    toggleModule(args[1]);
    if(isModuleActive(args[1])) printf("[47m[90m %s [37m[49m have been enabled.\n", args[1]);
    else printf("[47m[90m %s [37m[49m have been disabled.\n", args[1]);
  } else {
    putErr("Module not found. Try --list to display them.");
  }
  return 0;
}

int endJob(char **args) {
  if(!args[1] || strcmp(args[1], "--help") == 0) {
    putstr("[47m[90m ");
    putstr("end");
    putstr(" [37m[49m ");
    putstr("<num> End the given job.");
    _putchar('\n');
  } else {
    bgJob *b;
    int i = ft_atoi(args[1]);
    if(i>0) {
      i = pidOfBgJob(i);
      return (i<1)?(putErr("Job not found")):(!killJob(i));
    } else putErr("Invalid option");
  }
  return 0;
}

int showBgJobs() {
  bgJob *b;
  int i = 1;
  updateJobs();
  if(bgJobs == NULL) return putErr("No running jobs");
  putstr("[47m[90m ");
  putstr("Jobs");
  putstr(" [37m[49m ");
  putstr("Current running jobs :");
  _putchar('\n');
  for(b=bgJobs; b!=NULL; b=b->next, i++) {
    putstr("[47m[90m ");
    printf("%d [37m[49m ", i);
    //printf("%s\t%d\n", b->line , b->pid);
    printf("\t%d\n", b->pid);
  }
  return 0;
}

int _wait(char **args) {
  int status = 0;
  if(!args[1] || strcmp(args[1], "--help") == 0) {
    putstr("[47m[90m ");
    putstr("wait");
    putstr(" [37m[49m ");
    putstr("<time> Wait for a given amount of time.");
    _putchar('\n');
  } else {
    int i = ft_atoi(args[1]);
    if(i>0) sleep(i);
    else putErr("Invalid option");
  }

  return status;
}

int _eval(char **args) {
  if(!args[1] || strcmp(args[1], "--help") == 0) {
    putstr("[47m[90m ");
    putstr("eval");
    putstr(" [37m[49m ");
    putstr("<arg> Execute arguments as a shell command.");
    _putchar('\n');
    return 0;
  }
  char **com;
  int bg = 0;
  int fd[2], nbytes;
  pipe(fd);
  com = &args[1];
  struct cmd line [] = {{com}};
  int r = exec(fd, line, bg);
  close (fd [1]);
  return r;
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
  for (size_t i = 0; _DEFINED_FUNCTIONS[i]; i++) {
    putstr(" [47m[90m+[37m[49m ");
    putstr(_DEFINED_FUNCTIONS[i]);
    putstr("\n");
    /* code */
  }
  return 0;
}

/* ENDS HERE */

int notBuiltIn(char **argv) {
  if(strcmp(argv[0], "exit") == 0) return -1;
  else if(strcmp(argv[0], "env") == 0) return (printenv());
  else if(strcmp(argv[0], "help") == 0) return (help());
  else if(strcmp(argv[0], "cd") == 0) return (cd(argv));
  else if(strcmp(argv[0], "jobs") == 0) return (showBgJobs());
  else if(strcmp(argv[0], "end") == 0) return (endJob(argv));
  else if(strcmp(argv[0], "wait") == 0) return (_wait(argv));
  else if(strcmp(argv[0], "tg") == 0) return (tg(argv));
  else if(strcmp(argv[0], "eval") == 0) return (_eval(argv));
  return 1;
}

int processOne(char cmd[]) {
  int fd[2], nbytes;
  int i = 0;
  int rtrn = 0;
  int bg = 0;
  char * split;
  char * pipes;
  char *args[UCHAR_MAX];
  char **com;

  pipes = strtok(cmd, "|");
  while (pipes != NULL && i+1<UCHAR_MAX)
  {
    //printf("%s\n", pipes);
    strncat(pipes, "\0", sizeof(pipes)+1);
    pipes = strtok(NULL, "|");
  }

  pipe(fd);

  split = strtok(cmd, " ");
  while (split != NULL && i+1<UCHAR_MAX)
  {
    args[i++] = split;
    strncat(split, "\0", sizeof(split)+1);
    split = strtok(NULL, " ");
  }
  if(strcmp(args[i-1],"&")==0) {
	args[i-1]= 0;
	bg = 1;
  } else {
    args[i]=0;
  }
  if(i>=0) {
    if(i==0) {
      com = args;
      //com = malloc(2*sizeof(*cmd));
      com[0] = cmd;
      com[1] = 0;
    } else {
      com = args;
      //com = malloc(i*sizeof(*cmd));
      size_t n;
      for (n = 0; n < i; n++) {
        com[n] = args[n];
      }
      com[n+1] = 0;
    }
    struct cmd line [] = {{com}};

    rtrn = exec(fd, line, bg);
  	if(rtrn == -1 && errno == ENOENT) {
  		printf("not found : %s \n", args[0]);
  	}
  }
  close (fd [1]);
  return rtrn;
}

void testSub(char * cmd) {

  int fd[2], nbytes;

  pipe(fd);
  char *ls[] = { cmd };
  struct cmd line [] = {{ls}};
  exec(fd, line, 0);
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
  //runJjob(makeJjob(input), 1);
  //return 0;
  return processOne(input);
}

int main(int argc, char const *argv[], char **envp) {
  /*while (*envp)
      printf("%s\n", *envp++);*/
  ENV = envp;

  char *input;
  int rtrn;

  _term = STDIN_FILENO;
  _itty = isatty(_term);

  _pgid = getpid();
  if(setpgid(_pgid, _pgid) <0) {
    perror("Could not create process group");
    exit(1);
  }

  tcsetpgrp(_term, _pgid);
  tcgetattr(_term, &_tmodes);

  //testSub("ls");
  //testJob();

  signal(SIGINT, handler);
  while(run) {
    prompt();
    rtrn = readInput();
    if(rtrn<0) run=0;
  }
  terminate();
  return 0;
}
