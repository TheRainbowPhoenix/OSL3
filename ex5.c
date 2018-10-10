#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

int cnt = 0;

void boom(int s) 
{ 
	printf("Bonjour\n");
	cnt++;
	alarm(1);
	if(cnt>=5) {
		printf("Au revoir\n");
		cnt = 0;
	}  
}

int main()
{
	signal(SIGALRM, boom);  

	alarm(1);
	while(1) {
		pause();
	}

} 
