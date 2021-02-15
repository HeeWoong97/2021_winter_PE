#ifndef __CALCLOCK_H__
#define __CALCLOCK_H__

#include <linux/time64.h>

#define BILLION	1000000000

unsigned long long calclock(struct timespec64 *spclock, unsigned long long *total_time, unsigned long long *total_count);

#endif
