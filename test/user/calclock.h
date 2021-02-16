#ifndef __CALCLOCK_H__
#define __CALCLOCK_H__

#include <time.h>

unsigned long long calclock(struct timespec *spclock, unsigned long long *total_time, unsigned long long *total_count);

#endif
