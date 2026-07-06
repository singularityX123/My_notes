#include <stdio.h>
void delay(int i) {
	int t;
	while(i--) {
		t = 1000*1000;
		while(t--);
	}
}
int main(int argc, const char *argv[])
{
	while(1) {
		printf("1"); 
		fflush(stdout);
		delay(1000);
	}
	return 0;
}