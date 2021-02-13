#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/time.h>

#define BILLION	1000000000

unsigned long long calclock(struct timespec *spclock, unsigned long long *total_time, unsigned long long *total_count)
{
	long temp, temp_n;
	unsigned long long timedelay = 0;
	if (spclock[1].tv_nsec >= spclock[0].tv_nsec) {
		temp = spclock[1].tv_sec - spclock[0].tv_sec;
		temp_n = spclock[1].tv_nsec - spclock[0].tv_nsec;
		timedelay = BILLION * temp + temp_n;
	}
	else {
		temp = spclock[1].tv_sec - spclock[0].tv_sec - 1;
		temp_n = BILLION + spclock[1].tv_nsec - spclock[0].tv_nsec;
		timedelay = BILLION * temp + temp_n;
	}

	__sync_fetch_and_add(total_time, timedelay);
	__sync_fetch_and_add(total_count, 1);

	return timedelay;
}

int __init calclock_mod_init(void)
{
	int i;
	struct timespec spclock[2];
	unsigned long long time;
	unsigned long long count;
	int a = 0;

	printk("module init...");

	getrawmonotonic(&spclock[0]);
	for (i = 0; i < 100000; i++) {
		a++;
	}
	getrawmonotonic(&spclock[1]);

	calclock(spclock, &time, &count);
	printk("time: %llu, count: %llu", time, count);

	return 0;
}

void __exit calclock_mod_exit(void)
{
	printk("module exit...");
}

module_init(calclock_mod_init);
module_exit(calclock_mod_exit);
MODULE_LICENSE("GPL");
