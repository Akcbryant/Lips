#include <stdio.h>

void print_N_worlds(int N);

int main(int argc, char** argv) {
	int i;

	//print 5 with for
	for (i = 1; i <= 5; i++) {
		printf("%d: Hello, world!\n", i);
	}

	//print 5 with while
	i = 1;
	while(i <= 5) {
		printf("%d: Hello, world!\n", i);
		i++;
	}

	//print number of times the person says
	printf("How many times? ");
	scanf("%d", &i);
	print_N_worlds(i);
}

void print_N_worlds(int N) {
	int i;
	for (i = 1; i <= N; i++) {
		printf("%d: Hello World!\n", i);
	}
}