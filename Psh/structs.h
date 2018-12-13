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
void _readf(char *fname);

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
process * makeEmptyProcess();

typedef struct job
{
  struct job *next;
  int id;
  int valid;
  process *head;
  pid_t pgid;
  char stopping;
  struct termios tmodes;
  char* infile;
  char * outfile;
  int in, out, err;
  int wmode; //00 direct -- X1 seek in -- 1X seek out
  int fg;
} job;

int addJob(pid_t pgid, process * p);
job* makeEmptyJob();
job * makeJob(job *next, process *p, pid_t pgid, char s, struct termios tm, int in ,int out, int err);
void runBuiltin(process *p, int _in, int _out, int _err);
void runJob(job *j, int fg, int *id);
void waitJob(job *j);
void dumpJob(job *j);
void jobFg(job *j, int cnt);
void jobBg(job *j, int cnt);
void traceJob(job *j, char * status);
void notifyJobs();
void waitfor(pid_t pid);

char **ENV;
job *head;
pid_t _pgid;
struct termios _tmodes;
int _term;
int _itty;

int isBuiltin(process *p);
int changeDir(char * path);
char * _getENV(char *var);

void putstr(char *s);
void _putchar(char c);
int startsWith(char * p, char * s);
int checkPath(char * path);

/* BUILTIN */
int cd(char **args);

int jobs_main(int argc, char *argv[]);
int exec_main(int argc, char *argv[]);
int eval_main(int argc, char *argv[]);
int builtin_main(int argc, char *argv[]);
