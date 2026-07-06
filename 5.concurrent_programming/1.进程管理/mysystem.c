/*       实现思路：
*	1. fork()创建子进程
*	2. execl()进行进程替换
*	3. wait()等待子进程结束
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int mysystem(const char *command) {
	//1.fork()创建子进程
	switch(fork()) {
		case -1:
			perror("fork");
			return -1;
		case 0:
			//2.execl()进行进程替换
			execl("/bin/sh", "sh", "-c", command, (char *) NULL);
			perror("execl");
			_exit(1);
		default:
	}


	int wstatus;
	pid_t wpid;
	
	//3.wait()等待子进程结束
	wpid = wait(&wstatus);
	if(wpid < 0) {

		perror("wait");

		return wpid;
	}

	if( WIFEXITED(wstatus) ) {

		return WEXITSTATUS(wstatus);

	} else if ( WIFSIGNALED(wstatus) ) {

		return WTERMSIG(wstatus);

	} else {

		return 0;

	}

	return 0;
}

int main(int argc, const char *argv[])
{
	char buf[1024] = {};
    while(1){
        printf("输入命令-->");

        fgets(buf, sizeof(buf), stdin); 
		mysystem(buf);

    }

	return 0;
}