/*
 * PSH structs
 *
 * Dunno chat else should be wrote here ...
 */

#include <sys/types.h>
#include <termios.h>

#define FORK_FG 0
#define FORK_BG 1
#define FORK_NOJOB 2

#define WAIT_ANY (-1) //ANDROID NDK WTF ???

void trace(char[], char[]);
void run(char **argv, char **envp);
static void exec(char *, char **, char **);
int notBuiltIn(char **);
char ** environment();

typedef struct process
{
  struct process *next;
  char **argv;
  pid_t pid;
  char completed;
  char stopped;
  int status;
} process;

process * makeProcess(process * next, char **argv, pid_t pid, char c, char s, char st);

typedef struct job
{
  struct job *next;
  char *command;
  process *head;
  pid_t pgid;
  char stopping;
  struct termios tmodes;
  int in, out, err;
} job;

int addJob(pid_t pgid, char * command, process * p);
job * makeJob(job *next, char * command, process *p, pid_t pgid, char s, struct termios tm, int in ,int out, int err);
void waitJob(job *j);
void jobFg(job *j, int cnt);
void jobBg(job *j, int cnt);

char **ENV;
job *head;
pid_t _pgid;
struct termios _tmodes;
int _term;
int _itty;


int jobs_main(int argc, char *argv[]);
int exec_main(int argc, char *argv[]);
int eval_main(int argc, char *argv[]);
