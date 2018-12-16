#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char ** argv) {
	int p = (argc>1)?atoi(argv[1]):0;
	if(p<=4 && p>0) {
		int file;
		file = open("base.dat", O_RDWR, 0644);
		if(file<0) {perror("file"); exit(-1);}

		char *str;
		char c;
		lseek(file, (p-1), SEEK_SET);
		read(file, &c, 1);
		c = (c>0)?c-1:0;
		printf("%d\n", c);
		lseek(file, -1, SEEK_CUR);
		write(file,&c,sizeof(c));
		lseek(file, 0, SEEK_CUR);
		
		close(file);
	} else {
		printf("%s [1-4]\n",argv[0]);
	}
	return 0;
}
