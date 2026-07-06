#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, const char *argv[])
{
	switch(fork()) {
	case -1:
		perror("fork");
		exit(EXIT_FAILURE);
		break;
	case 0:
		sleep(1);
		break;
	default:
		if( wait(NULL) < 0) {
			perror("wait");
			exit(EXIT_FAILURE);
		}
	}
	return 0;
}