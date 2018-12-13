
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "structs.h"

extern char **environ;
char *paths;
static int last_path_index = 0;

static int last_exec_err = 0;

/*
 * Real stuff begins here
 */

static void pathParse(char * p) {
  paths = (char *) malloc((strlen(p)) * sizeof(char*));
  strcpy(paths, &p[5]);
}

char * pathStep(char *cmd) {
  if(paths == NULL) return NULL;
  int b, e; //path begin & end indexes
  e = b = last_path_index;
  while(paths[e]!='\0' && paths[e] != ':' && paths[e] != '%') e++;
  if(b==e) return NULL;
  //printf("%s %d\n", cmd, strlen(cmd));
  int l = e-b+strlen(cmd)+2;
  char * s = (char *) malloc(l*sizeof(char*));
  strncpy(s, paths+b, e-b);
  strcat(s, "/");
  strcat(s, cmd);
  strcat(s, "\0");
  last_path_index = e+1;
  return s;
}

char ** environment() {
  char **envp;
  char p[] = "PATH";
  for (envp = environ ; *envp ; envp++) {
    if (strchr(*envp, '=')) {
      if(strncmp (p,*envp, 4) == 0) {
        pathParse(*envp);
        //printf("%s\n", paths);
      }
    }
  }
  envp = environ;
  return envp;
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

void run(char **argv, char **envp) {
  char *tryCmd;
  int e;
  if (strchr(argv[0], '/') != NULL) {
    exec(argv[0], argv, envp);
    e = errno;
  } else {
    e = ENOENT; //No such file or dir
    while((tryCmd = pathStep(argv[0])) != NULL) {
      //printf("%s\n", tryCmd);
      exec(tryCmd, argv, envp);
      if(errno != ENOENT && errno != ENOTDIR) e = errno;
    }
    free(tryCmd);
  }
  struct stat infos;
  if (errno != ENOEXEC) {
    printf("err\n");
    if((stat(argv[0], &infos) == 0) && (S_ISDIR(infos.st_mode))) {
      last_exec_err = 21; //EISDIR
      trace(argv[0], "directory");
    }
    else {
      switch (e) {
        case ENOENT: last_exec_err = 127; break;
        case EACCES: last_exec_err = 126; break;
        default: last_exec_err = 2; break;
      }
      trace(argv[0], strerror(e));
    }
  } else {
    int fd  = open(argv[0], O_RDONLY);
    if(fd!=-1) {
      char header[80]; // read the 80 firsts chars, searching for a shell insctruction
      int header_l = read(fd, &header[0], 80);
      close(fd);
      if(header_l == 0) return ;//0; // empty file, nothing to do
      if(header_l > 0 && header[0] == '#' && header[1] == '!') {
        trace(argv[0], "is a script file, which is not implemented");
        //TODO: exec script !
      } else if((header_l != -1) && header_l >4 && header[0]==127 && header[1]=='E' && header[2]=='L' && header[3]=='F') {
        trace(argv[0], "cannot execute binary file"); // Exec format error
      } else if((header_l != -1) && header_l >3 && header[0]=='M' && header[1]=='Z' && header[2]==144) {
        trace(argv[0], "cannot execute windows binary file");
      }
    } else {
      trace(argv[0], "File busy or unavailable");
      last_exec_err = 26; //ETXTBSY
    }
  }
  trace(argv[0], strerror(e));
}

static void exec(char *cmd, char ** argv, char ** envp) {
  int e;
  execve(cmd, argv, envp);
  e = errno;
  if (e == ENOEXEC) {
    printf("ENOEXEC\n");
  }
  errno = e;
}

/*
 * Is that unit test ???
 */


int exec_main(int argc, char *argv[]) {
  printf("[47m[90m EXEC MAIN TEST [37m[49m\n");
  char** exe = (char *[]){"ech64", "hello", "yay", NULL};
  char** exe2 = (char *[]){"/bin/ls", "-la", NULL};
  char** exe3 = (char *[]){"ls", "-la", NULL};
  char** exe4 = (char *[]){"test.sh", NULL};

  char** envp = environment();

  printf("%s\n", _getENV("HOME"));

  _readf(exe4[0]);
  pid_t pid;
  if((pid=fork()) == -1) trace("fork()","Fork error");
  if(pid==0) run(exe4, envp);
  else wait(NULL);
  //exec(exe[0], exe, envp);
  return 0;
}
