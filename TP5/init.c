#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char ** argv) {
	int file;
	file = open("base.dat", O_CREAT|O_WRONLY|O_TRUNC, 0644);
	if(file<0) {perror("file"); exit(-1);}

	char d = 100;
	for(int i=0; i<4; i++) {
		write(file,&d,sizeof(d));
	}
	close(file);
	return 0;
}
