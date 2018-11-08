#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig) {
  printf("Ceci est un bon exercice pour comprendre le mécanisme des signaux!\n");
}

int main(int argc, char const *argv[]) {
  int run = 1;

  if (signal(SIGUSR1, handler) == SIG_ERR) {
		printf("Failed !\n");
	}

  printf("PID: %d\n", getpid());
  while(run) {
    sleep(1);
  }

  /* code */
  return 0;
}
