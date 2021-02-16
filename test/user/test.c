#include <stdio.h>
#include <time.h>
#include "calclock.h"

int a;

void print_global(void)
{
	printf("global: %d\n", a);
}

void print_local(int num)
{
	printf("local: %d\n", num);
}

int main(void)
{
	struct timespec spclock1[2];
	struct timespec spclock2[2];
	unsigned long long time1, count1;
	unsigned long long time2, count2;

	int n = 1;
	a = 1;

	clock_gettime(CLOCK_REALTIME, &spclock1[0]);
	print_global();
	clock_gettime(CLOCK_REALTIME, &spclock1[1]);

	clock_gettime(CLOCK_REALTIME, &spclock2[0]);
	print_local(n);
	clock_gettime(CLOCK_REALTIME, &spclock2[1]);

	calclock(spclock1, &time1, &count1);
	calclock(spclock2, &time2, &count2);

	printf("time taken to print global variable: %llu nsec\n", time1);
	printf("time taken to print local variable: %llu nsec\n", time2);

	return 0;
}
