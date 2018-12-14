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

char * prev = "exit";

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

process * parseProcess(char *args, job **j) {
  process *p;
  //char ** exe;
  char *cmd[UCHAR_MAX];
  //char *a[UCHAR_MAX] = {};
  char buff[UCHAR_MAX];
  char *c;
  char *pos = args;

  //prev = args;

  //int i = 0;
  int sz = 0;

  p = makeEmptyProcess();

  while(*pos++ != '\0') if(*pos == ' ') sz++;
  p->argv = (char**)malloc((sz + 1) * sizeof(char*));

  //p->argv = malloc((UCHAR_MAX)*sizeof(char*));
  //char **cmd = p->argv;

  p->argv[0] = args;
  int i = 1;
  char* hit = args;

  while((hit = strchr(hit, ' ')) != NULL) {
    //while(*hit==' ' && *hit) hit++;
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

  prev = raw;
  strcpy(prev, raw);

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
    //printf("%d ", n);
    i = -1;
    while(pipes != NULL && i+1<UCHAR_MAX) {
      while(*pipes==' ') pipes++;

      jobs[++i] = pipes;

      if(p!=NULL) op = p;
      p = parseProcess(pipes, &jb);
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
    //if(i > 0)  printf(" (%d)\n", i);
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
  char *p = calloc(1, strlen(prev)+1); //strlen(exit)+1
  int sz;
  printf("\33[2K\r%s%s", ps1, prev);
  sz = sprintf(p, "%s", prev);
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

int main(int argc, char *argv[]) {
  int run = 1;

  init();

  loop(run); //YES IT PARSES LIKES *** => ALL THE TESTS ARE HERE AND WORKING

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
