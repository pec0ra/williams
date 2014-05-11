/*
 * ElementalX msm-sleeper by flar2 <asegaert@gmail.com>
 * 
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/earlysuspend.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <mach/cpufreq.h>

#define MSM_SLEEPER_MAJOR_VERSION	2
#define MSM_SLEEPER_MINOR_VERSION	0

extern uint32_t maxscroff;
extern uint32_t maxscroff_freq;
static uint32_t policy_max_freq;
static uint32_t old_limited_max_freq;

int is_sleeping = 0;


extern uint32_t limited_max_freq;

#ifdef CONFIG_HAS_EARLYSUSPEND

static int update_cpu_max_freq(int cpu, uint32_t max_freq)
{
	int ret = 0;

	ret = msm_cpufreq_set_freq_limits(cpu, MSM_CPUFREQ_NO_LIMIT, max_freq);
	if (ret)
		return ret;
	
	limited_max_freq = max_freq;
	
#ifdef DEBUG_CPU_LIMITER
	if (max_freq != MSM_CPUFREQ_NO_LIMIT)
		pr_info("%s: Limiting cpu%d max frequency to %d\n",
			__func__, cpu, max_freq);
	else
		pr_info("%s: Max frequency reset for cpu%d\n",
			__func__, cpu);
#endif
	ret = cpufreq_update_policy(cpu);

	return ret;
}

static void __cpuinit msm_sleeper_early_suspend(struct early_suspend *h)
{
	int cpu;
	int i;
	int num_cores = 2;
	int ret = 0;
	
	
#ifdef CONFIG_MSM_CPUFREQ_LIMITER
		if(limited_max_freq != MSM_CPUFREQ_NO_LIMIT)
			old_limited_max_freq = limited_max_freq;
		else
			old_limited_max_freq = policy_max_freq;
#else
		old_limited_max_freq = policy_max_freq;
#endif
	pr_info("Store last max freq: %d\n", old_limited_max_freq);
	for_each_possible_cpu(cpu) {
		ret = update_cpu_max_freq(cpu, maxscroff_freq);
				if (ret)
					pr_debug("Unable to limit cpu%d max freq to %d\n",
							cpu, maxscroff_freq);
		pr_info("Limit max frequency to: %d\n", maxscroff_freq);
	}

	for (i = 1; i < num_cores; i++) {
		if (cpu_online(i))
			cpu_down(i);
	}
	
	is_sleeping = 1;

	return; 
}

static void __cpuinit msm_sleeper_late_resume(struct early_suspend *h)
{
	int cpu;
	int i;
	int num_cores = 2;
	int ret = 0;

	for_each_possible_cpu(cpu) {
		ret = update_cpu_max_freq(cpu, old_limited_max_freq);
				if (ret)
					pr_debug("Unable to limit cpu%d max freq to %d\n",
							cpu, old_limited_max_freq);
		pr_info("Restore max frequency to %d\n", old_limited_max_freq);
	}

	for (i = 1; i < num_cores; i++) {
		if (!cpu_online(i))
			cpu_up(i);
	}
	
	is_sleeping = 0;

	return; 
}

void msm_sleeper_add_limit(uint32_t max){
	policy_max_freq = max;
}

static struct early_suspend msm_sleeper_early_suspend_driver = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 10,
	.suspend = msm_sleeper_early_suspend,
	.resume = msm_sleeper_late_resume,
};
#endif

static int __init msm_sleeper_init(void)
{
	pr_info("msm-sleeper version %d.%d\n",
		 MSM_SLEEPER_MAJOR_VERSION,
		 MSM_SLEEPER_MINOR_VERSION);

#ifdef CONFIG_HAS_EARLYSUSPEND
		register_early_suspend(&msm_sleeper_early_suspend_driver);
#endif
	return 0;
}

MODULE_AUTHOR("flar2 <asegaert@gmail.com>");
MODULE_DESCRIPTION("'msm-sleeper' - Limit max frequency and shut down cores while screen is off");
MODULE_LICENSE("GPL v2");

late_initcall(msm_sleeper_init);

