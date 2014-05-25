/*
 * Author: Paul Reioux aka Faux123 <reioux@gmail.com>
 *
 * Copyright 2012-2014 Paul Reioux
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

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#include <linux/cpu.h>
#include <linux/cpufreq.h>

#include <mach/cpufreq.h>

#define MSM_CPUFREQ_LIMIT_MAJOR		1
#define MSM_CPUFREQ_LIMIT_MINOR		2

#define DEBUG_CPU_LIMITER 1

struct freq_limiter{
	uint32_t max;
};

DEFINE_PER_CPU(struct freq_limiter, limited_max_freq);
static uint32_t default_max_freq = 1782000;

static int update_cpu_max_freq(int cpu, uint32_t max_freq)
{
	int ret = 0;

	struct freq_limiter *limit = &per_cpu(limited_max_freq, cpu);
	ret = msm_cpufreq_set_freq_limits(cpu, MSM_CPUFREQ_NO_LIMIT, max_freq);
	if (ret)
		return ret;

	limit->max = max_freq;

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

static ssize_t msm_cpufreq_limit0_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct freq_limiter *limit = &per_cpu(limited_max_freq, 0);
	return sprintf(buf, "%u\n", limit->max);
}

static ssize_t msm_cpufreq_limit1_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct freq_limiter *limit = &per_cpu(limited_max_freq, 1);
	return sprintf(buf, "%u\n", limit->max);
}

static ssize_t msm_cpufreq_limit0_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int cpu = 0;
	int ret = 0;
	int max_freq;
	unsigned int data;

	if(sscanf(buf, "%u", &data) == 1) {
		max_freq = data;
		ret = update_cpu_max_freq(cpu, max_freq);
		if (ret)
			pr_debug("Unable to limit cpu%d max freq to %d\n",
					cpu, max_freq);
	}

	return count;
}

static ssize_t msm_cpufreq_limit1_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int cpu = 1;
	int ret = 0;
	int max_freq;
	unsigned int data;

	if(sscanf(buf, "%u", &data) == 1) {
		max_freq = data;
		ret = update_cpu_max_freq(cpu, max_freq);
		if (ret)
			pr_debug("Unable to limit cpu%d max freq to %d\n",
					cpu, max_freq);
	}

	return count;
}

static ssize_t msm_cpufreq_limit_version_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "version: %u.%u\n",
			MSM_CPUFREQ_LIMIT_MAJOR, MSM_CPUFREQ_LIMIT_MINOR);
}

static struct kobj_attribute msm_cpufreq_limit0_attribute =
	__ATTR(cpufreq_limit0, 0666,
		msm_cpufreq_limit0_show,
		msm_cpufreq_limit0_store);

static struct kobj_attribute msm_cpufreq_limit1_attribute =
	__ATTR(cpufreq_limit1, 0666,
		msm_cpufreq_limit1_show,
		msm_cpufreq_limit1_store);

static struct kobj_attribute msm_cpufreq_limit_version_attribute = 
	__ATTR(msm_cpufreq_limit_version, 0444 ,
		msm_cpufreq_limit_version_show,
		NULL);

static struct attribute *msm_cpufreq_limit_attrs[] =
	{
		&msm_cpufreq_limit0_attribute.attr,
		&msm_cpufreq_limit1_attribute.attr,
		&msm_cpufreq_limit_version_attribute.attr,
		NULL,
	};

static struct attribute_group msm_cpufreq_limit_attr_group =
	{
		.attrs = msm_cpufreq_limit_attrs,
	};


static struct kobject *msm_cpufreq_limit_kobj;

static int msm_cpufreq_limit_init(void)
{
	int cpu;
	int sysfs_result;
	struct freq_limiter *limit;

	for_each_possible_cpu(cpu){
		limit = &per_cpu(limited_max_freq, cpu);
		limit->max = default_max_freq;
		pr_info("[PCL] limit : %d , default_max_freq : %d , %d\n", limit->max, default_max_freq, (uint32_t) &default_max_freq);
	}

	msm_cpufreq_limit_kobj =
		kobject_create_and_add("msm_cpufreq_limit", kernel_kobj);
	if (!msm_cpufreq_limit_kobj) {
		pr_err("%s msm_cpufreq_limit_kobj kobject create failed!\n",
			__func__);
		return -ENOMEM;
        }

	sysfs_result = sysfs_create_group(msm_cpufreq_limit_kobj,
			&msm_cpufreq_limit_attr_group);

        if (sysfs_result) {
		pr_info("%s msm_cpufreq_limit_kobj create failed!\n",
			__func__);
		kobject_put(msm_cpufreq_limit_kobj);
	}
	return sysfs_result;
}

static void msm_cpufreq_limit_exit(void)
{
	if (msm_cpufreq_limit_kobj != NULL)
		kobject_put(msm_cpufreq_limit_kobj);
}

module_init(msm_cpufreq_limit_init);
module_exit(msm_cpufreq_limit_exit);
MODULE_LICENSE("GPL v2"); 
MODULE_AUTHOR("Paul Reioux <reioux@gmail.com>");
MODULE_DESCRIPTION("Krait CPU frequency Limit Driver");

