#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>
#include <linux/timer.h>

#define BILLION	1000000000

unsigned long long calclock(struct timespec64 *spclock, unsigned long long *total_time, unsigned long long *total_count)
{
	long temp, temp_n;
	unsigned long long timedelay = 0;

	printk("spclock[0].tv_sec: %lld", spclock[0].tv_sec);
	printk("spclock[1].tv_sec: %lld", spclock[1].tv_sec);

	printk("spclock[0].tv_nsec: %ld", spclock[0].tv_nsec);
	printk("spclock[1].tv_nsec: %ld", spclock[1].tv_nsec);

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
	printk("temp: %ld", temp);
	printk("temp_n: %ld", temp_n);
	printk("timedelay: %lld", timedelay);

	__sync_fetch_and_add(total_time, timedelay);
	__sync_fetch_and_add(total_count, 1);

	return timedelay;
}

int __init calclock_mod_init(void)
{
	int i;
	ktime_t start, end;
	struct timespec64 spclock[2];
	unsigned long long time = 0;
	unsigned long long count = 0;
	int a = 0;

	printk("module init...");

	ktime_get_ts64(&spclock[0]);
	start = ktime_get();
	for (i = 0; i < 100000; i++) {
		a++;
	}
	ktime_get_ts64(&spclock[1]);
	end = ktime_get();

	calclock(spclock, &time, &count);
	printk("time: %llu, count: %llu", time, count);
	printk("time: %lld", end - start);

	return 0;
}

void __exit calclock_mod_exit(void)
{
	printk("module exit...");
}

module_init(calclock_mod_init);
module_exit(calclock_mod_exit);
MODULE_LICENSE("GPL");
