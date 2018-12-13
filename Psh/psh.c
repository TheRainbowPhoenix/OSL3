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

typedef struct fdef {
  int fd;
  int fw;
} fdef;

fdef *checkOut(char**);

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

/* TODO: move to parsers */

/*
 * Supported chars
 *
 * '\n'=> newline
 * ' ' => seprarate process args
 * '\''=> args block
 * '&' =>  backgnd / (+ '&' == AND)
 * '|' => pipe (job processes) / (+ '|' == OR)
 * ';' => 'Job' separator
 * '(' => Begin block
 * ')' => End   -----
 * '#' => comment (ignore line except first)
 * '\\'=> add another line
 */
// echo aa | tr 'a' 'A' >>out; cat out;
//   p0    |   p1       +> f
//   p0

process * parseProcess(char *args, int *fg) {
  process *p;
  //char ** exe;
  char *cmd[UCHAR_MAX];
  //char *a[UCHAR_MAX] = {};
  char buff[UCHAR_MAX];
  char *c;
  char *pos = args;

  //int i = 0;
  int sz = 0;

  p = makeEmptyProcess();

  while(*pos++ != '\0') if(*pos == ' ') sz++;
  p->argv = (char**)malloc((sz + 1) * sizeof(char*));

  //p->argv = malloc((UCHAR_MAX)*sizeof(char*));
  //char **cmd = p->argv;

  p->argv[0] = args;
  int i = 0;
  char* hit = args;
  while((hit = strchr(hit, ' ')) != NULL) {
    *hit++ = '\0';
    p->argv[i++] = hit;
  }


  return p;

  /*cmd[j] = strtok(args," ");

  while(cmd[j]!=NULL)
  {
    printf("%s\n", cmd[j]);
     cmd[++j] = strtok(NULL," ");
  }*/
  //cmd[j] = '\0';

  /*int i = 0;
  int j = 0;
  int flg = 0;
  while (*args != '\0' && i<UCHAR_MAX) {
    //buff[i] = *args;
    switch (*args) {
      case '&': fg = 0; break;
      case ' ': while(*args==' ' && *args) args++;
        buff[i] = '\0';
        printf("%s\n", buff);
        cmd[j] = buff;
        //exe = realloc(exe, j+2 * sizeof(char *));
        //exe[j] = calloc(1, i+1 );
        //memcpy(exe[j], buff, i+1);
        i = 0;
        j++;
        break;
      default: buff[i] = *args;
    }
    args++;
    i++;
  }*/
  /*exe = malloc(j*sizeof(char *));
  for (int n = 0; n <= j; n++) {
    exe[n] = cmd[n];
  }*/
  //buff[i] = '\0';
  //printf("b: %s\n", buff);
  //exe[j] = calloc(1, i+1 );
  //memcpy(exe[j], buff, i+1);
}

int parseCmd(char raw[]) {
  int n = -1;
  int i = 0;
  int j = -1;
  int skip = 0;
  char * token;
  char * pipes;
  char * split;
  char * commands[UCHAR_MAX];
  char * jobs[UCHAR_MAX];
  char * procs[UCHAR_MAX];

  pid_t pgid = getpid();
  job * jb;
  process *p = NULL;
  process *op = NULL;
  char** exe;
  int id = 1;
  //token = strtok(raw, ";");

  token = strtok(raw, ";");
  while(token != NULL && n+1<UCHAR_MAX) {
    commands[++n] = token;
    while(*token==' ') token++;
    //printf("%d : %s\n", n, token);
    token = strtok(NULL, ";");
  }
  commands[++n] = '\0';
  for (n=0; commands[n] != '\0'; n++) {

    jb = makeEmptyJob();

    pipes = strtok(commands[n], "|");
    printf("%d ", n);
    i = -1;
    while(pipes != NULL && i+1<UCHAR_MAX) {
      while(*pipes==' ') pipes++;

      jobs[++i] = pipes;

      if(p!=NULL) op = p;
      p = parseProcess(pipes, &jb->fg);
      if(jb->head==NULL) jb->head = p;
      if(op != NULL) op->next = p;

      //printf("%s |", pipes);
      pipes = strtok(NULL, "|");
    }
    //printf("%s\n", jb->head->argv[0]);
    runJob(jb, jb->fg, &id);

    if(op != NULL) op=NULL;
    if(p != NULL) p=NULL;
    if(jb != NULL) jb=NULL;
    if(i > 0)  printf(" (%d)\n", i);
      //printf("%s\n", commands[i]);


  }

  /*
  token = strtok(raw, ";");
  while(token != NULL && n+1<UCHAR_MAX) {
    commands[n++] = token;
    pipes = strtok(token, "|");
    while (pipes != NULL && i+1<UCHAR_MAX) {
      jobs[i++] = pipes;
      split = strtok(pipes, " ");
      while (split != NULL && j+1<UCHAR_MAX) {
        procs[j++] = split;
        printf("%s ", split);
        split = strtok(NULL, " ");
      }
      pipes = strtok(NULL, "|");
    }
    token = strtok(NULL, ";");
  }
  */
  //printf("%d\n", n);

  if(strcmp(raw,"exit")==0) {
    return -1;
  } else {
    return 0;
  }
}

/* TODO : move to input */

#define VK_ESCP 27
#define VK_ESC2 91
#define VK_BKSP 127

/*
 * KEYS
 *
 * 9          : TAB
 * 27 91 65   : up   (27 91 49 51 65   : alt up)
 * 27 91 66   : down
 * 27 91 67   : ->
 * 27 91 68   : <-
 * 27         : echap
 * 59         : ;
 * 32         : space
 * 127        : suppr <==]
 * 126        : suppr [==>
 */

char * getPrevious() {
  char *p = calloc(1, 6); //strlen(exit)+1
  int sz;
  printf("\33[2K\r%sexit", ps1);
  sz = sprintf(p, "exit");
  return p;
}

int readInput() {
	int rc;
	char buffer;
	int i = -1;
	char input[1024]; //TODO : free !
  int cflag = 0;
  setbuf(stdout, 0);
	//while((rc = read(0, &buffer, 1)) && i++<UCHAR_MAX-1 && buffer != '\n') {
	while((buffer = getchar()) && i++<UCHAR_MAX-1 && buffer != '\n') {
    //printf("%d\n", buffer);
    if(buffer == VK_BKSP) {
      input[--i] = '\0';
      i--;
      printf("\r");
      for (size_t i = strlen(input)+strlen(ps1)+1; i >0; i--) {
        _putchar(' ');
      }
      printf("\r%s%s", ps1, input);
      fflush(stdout);
    } else {
      if(cflag == 2) {
        char *c = getPrevious();
        for (i = 0; i < ((c!=NULL)?strlen(c):0); i++) input[i] = c[i];
        input[i] = '\0';
        cflag = -1;
      }
      if(cflag == 1) {
        if(buffer == VK_ESC2) cflag = 2;
        else cflag = 0;
      } else if (cflag == 0){
        if(buffer == VK_ESCP) cflag = 1;
        else input[i] = buffer;
      } else {
        cflag = 0;
      }
    }
	}
	input[i] = '\0';
  //printf("%s\n", input);
  return parseCmd(input);

	//return i;
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

fdef *checkOut(char** elems) {
  int i = 0;
  fdef *fd = malloc(sizeof(fdef));
  while(elems[i] !=NULL) {
    if(strcmp(elems[i],">")==0) {
      fd->fd=open(elems[i+1],O_CREAT|O_TRUNC|O_WRONLY,0644);
      if(fd->fd<0) perror("open");
      fd->fw=STDOUT_FILENO;
      elems[i]=NULL;
      return fd;
    }
    if(strcmp(elems[i],"<")==0) {
      fd->fd=open(elems[i+1],O_CREAT|O_TRUNC|O_WRONLY,0644);
      if(fd->fd<0) perror("open");
      fd->fw=STDIN_FILENO;
      elems[i]=NULL;
      return fd;
    }
    i++;
  }
  return NULL;
}

void readline(char *line)
{
    fgets(line,1024,stdin);
    line[strcspn(line,"\n")]='\0';
}

void parseline(char *args,char **tokens)
{
  int i=0;
  tokens[0]=strtok(args," ");
  if(tokens[0]==NULL)
    return;
  do {
    if(i>=(sizeof(tokens)/sizeof(char*))-1)
      tokens=realloc(tokens,sizeof(char*)*((sizeof(tokens)/sizeof(char*)+64)));
    tokens[++i]=strtok(NULL," ");
  } while(tokens[i]!=NULL);

}

void waitfor(pid_t pid) {
  int status;
  tcsetpgrp(_term, pid);
  waitpid(pid, &status, WUNTRACED);
  if(WIFSTOPPED(status)) {
    // stuff
  }
  tcsetpgrp(_term, getpid());
}

void _exec(char **elems, fdef *fd) {
  pid_t pid = fork();
  if(pid==0) {
    signal (SIGINT, SIG_DFL);
    signal (SIGQUIT, SIG_DFL);
    signal (SIGTSTP, SIG_DFL);
    signal (SIGTTIN, SIG_DFL);
    signal (SIGTTOU, SIG_DFL);
    signal (SIGCHLD, SIG_DFL);
    setpgid(0,0);
    if(fd !=NULL) {
      dup2(fd->fd,fd->fw);
      close(fd->fd);
    }
    run(elems, ENV);
  } else if (pid<0) {
    fprintf(stderr, "Fork error\n");
  } else {
    setpgid(pid,pid);
    waitfor(pid);
    if(fd !=NULL) {
      free(fd);
    }
  }
}

int shellexec(char **elems) {
  fdef *fd = checkOut(elems);
  if(elems[0] == NULL) return 1;
  if(strcmp(elems[0],"exit")==0) {
    return 0;
  } else {
    _exec(elems, fd);
  }
  return 1;
}

int main(int argc, char *argv[]) {
  int run = 1;

  init();

  loop(run);
  /* remove this later */

  char *line = (char*)malloc(sizeof(char)*1024);
  char **elems =(char **)malloc(sizeof(char*)*64);
  while(0) {
    printf("%s",ps1);
    readline(line);
    parseline(line, elems);
    if(elems[0]==NULL) {
      run = 0;
    }
    run = shellexec(elems);
  }
  free(line);
  free(elems);

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
