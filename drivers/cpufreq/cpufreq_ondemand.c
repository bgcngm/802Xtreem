/*
 *  drivers/cpufreq/cpufreq_ondemand.c
 *
 *  Copyright (C)  2001 Russell King
 *            (C)  2003 Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>.
 *                      Jun Nakajima <jun.nakajima@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/slab.h>

/* Google systrace just supports Interactive governor (option -l)
* Just backport Interactive trace points for Ondemand governor use
*/
#define CREATE_TRACE_POINTS
#include <trace/events/cpufreq_interactive.h>

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */

#define DEF_SAMPLING_RATE				(50000)
#define DEF_FREQUENCY_DOWN_DIFFERENTIAL		(10)
#define DEF_FREQUENCY_UP_THRESHOLD		(80)
#define DEF_SAMPLING_DOWN_FACTOR		(1)
#define MAX_SAMPLING_DOWN_FACTOR		(100000)
#define MICRO_FREQUENCY_DOWN_DIFFERENTIAL	(3)
#define MICRO_FREQUENCY_UP_THRESHOLD		(95)
#define MICRO_FREQUENCY_MIN_SAMPLE_RATE		(10000)
#define MIN_FREQUENCY_UP_THRESHOLD		(11)
#define MAX_FREQUENCY_UP_THRESHOLD		(100)
#define MIN_FREQUENCY_DOWN_DIFFERENTIAL		(1)
#define DBS_INPUT_EVENT_MIN_FREQ		(1134000)
#define DEF_UI_DYNAMIC_SAMPLING_RATE		(30000)
#define DBS_UI_SAMPLING_MIN_TIMEOUT		(30)
#define DBS_UI_SAMPLING_MAX_TIMEOUT		(1000)
#define DBS_UI_SAMPLING_TIMEOUT			(80)
#define DBS_SWITCH_MODE_TIMEOUT		(1000)

/*
 * The polling frequency of this governor depends on the capability of
 * the processor. Default polling frequency is 1000 times the transition
 * latency of the processor. The governor will work on any processor with
 * transition latency <= 10mS, using appropriate sampling
 * rate.
 * For CPUs with transition latency > 10mS (mostly drivers with CPUFREQ_ETERNAL)
 * this governor will not work.
 * All times here are in uS.
 */
#define MIN_SAMPLING_RATE_RATIO			(2)

static unsigned int min_sampling_rate;
static unsigned int skip_ondemand = 0;

#define LATENCY_MULTIPLIER			(1000)
#define MIN_LATENCY_MULTIPLIER			(100)
#define TRANSITION_LATENCY_LIMIT		(10 * 1000 * 1000)

#define POWERSAVE_BIAS_MAXLEVEL			(1000)
#define POWERSAVE_BIAS_MINLEVEL			(-1000)

static void do_dbs_timer(struct work_struct *work);
static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				unsigned int event);

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMAND
static
#endif
struct cpufreq_governor cpufreq_gov_ondemand = {
       .name                   = "ondemand",
       .governor               = cpufreq_governor_dbs,
       .max_transition_latency = TRANSITION_LATENCY_LIMIT,
       .owner                  = THIS_MODULE,
};

/* Sampling types */
enum {DBS_NORMAL_SAMPLE, DBS_SUB_SAMPLE};

struct cpu_dbs_info_s {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_iowait;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;
	struct cpufreq_policy *cur_policy;
	struct delayed_work work;
	struct cpufreq_frequency_table *freq_table;
	unsigned int freq_lo;
	unsigned int freq_lo_jiffies;
	unsigned int freq_hi_jiffies;
	unsigned int rate_mult;
	unsigned int prev_load;
	unsigned int max_load;
	int input_event_freq;
	int cpu;
	unsigned int sample_type:1;
	/*
	 * percpu mutex that serializes governor limit change with
	 * do_dbs_timer invocation. We do not want do_dbs_timer to run
	 * when user is changing the governor or limits.
	 */
	struct mutex timer_mutex;
};
static DEFINE_PER_CPU(struct cpu_dbs_info_s, od_cpu_dbs_info);

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info);
static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info);

static unsigned int dbs_enable;	/* number of CPUs using this policy */

/* Workqueues handle frequency scaling */
static DEFINE_PER_CPU(struct task_struct *, up_task);
static spinlock_t input_boost_lock;
static bool input_event_boost = false;
static unsigned long input_event_boost_expired = 0;

#ifdef CONFIG_EARLYSUSPEND_BOOST_CPU_SPEED
extern int has_boost_cpu_func;
#endif

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_MULTI_PHASE
/* Scalable frequency change */
#define TABLE_SIZE			5
#define MAX(x,y)			(x > y ? x : y)
#define MIN(x,y)			(x < y ? x : y)
#define FREQ_NEED_BURST(x)	(x < 600000 ? 1 : 0)

static	struct cpufreq_frequency_table *tbl = NULL;
static unsigned int *tblmap[TABLE_SIZE] __read_mostly;
static unsigned int tbl_select[4];
static unsigned int up_threshold_level[2] __read_mostly = {95, 85};
static int input_event_counter = 0;
struct timer_list freq_mode_timer;

static inline void switch_turbo_mode(unsigned);
static inline void switch_normal_mode(void);
#endif

/*
 * dbs_mutex protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);

static struct dbs_tuners {
	unsigned int sampling_rate;
	unsigned int up_threshold;
	unsigned int up_threshold_multi_core;
	unsigned int down_differential;
	unsigned int down_differential_multi_core;
	unsigned int optimal_freq;
	unsigned int up_threshold_any_cpu_load;
	unsigned int sync_freq;
	unsigned int ignore_nice;
	unsigned int sampling_down_factor;
	int          powersave_bias;
	unsigned int io_is_busy;
#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_2_PHASE
	unsigned int two_phase_freq;
#endif
	unsigned int shortcut;
	unsigned int origin_sampling_rate;
	unsigned int ui_sampling_rate;
	unsigned int ui_timeout;
	unsigned int enable_boost_cpu;
} dbs_tuners_ins = {
	.up_threshold_multi_core = DEF_FREQUENCY_UP_THRESHOLD,
	.up_threshold = DEF_FREQUENCY_UP_THRESHOLD,
	.sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR,
	.down_differential = DEF_FREQUENCY_DOWN_DIFFERENTIAL,
	.down_differential_multi_core = MICRO_FREQUENCY_DOWN_DIFFERENTIAL,
	.up_threshold_any_cpu_load = DEF_FREQUENCY_UP_THRESHOLD,
	.ignore_nice = 0,
	.powersave_bias = 0,
	.sync_freq = 0,
	.optimal_freq = 0,
#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_2_PHASE
	.two_phase_freq = 0,
#endif
	.shortcut = 0,
	.ui_sampling_rate = DEF_UI_DYNAMIC_SAMPLING_RATE,
	.ui_timeout = DBS_UI_SAMPLING_TIMEOUT,
	.enable_boost_cpu = 1,
};

bool is_ondemand_locked(void)
{
	if((dbs_tuners_ins.powersave_bias == POWERSAVE_BIAS_MAXLEVEL) ||
	    (dbs_tuners_ins.powersave_bias == POWERSAVE_BIAS_MINLEVEL))
		return true;
	else
		return false;
}
EXPORT_SYMBOL(is_ondemand_locked);

static inline u64 get_cpu_idle_time_jiffy(unsigned int cpu, u64 *wall)
{
	u64 idle_time;
	u64 cur_wall_time;
	u64 busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

	busy_time  = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];

	idle_time = cur_wall_time - busy_time;
	if (wall)
		*wall = jiffies_to_usecs(cur_wall_time);

	return jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);
	else
		idle_time += get_cpu_iowait_time_us(cpu, wall);

	return idle_time;
}

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu, cputime64_t *wall)
{
	u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

	if (iowait_time == -1ULL)
		return 0;

	return iowait_time;
}

/*
 * Find right freq to be set now with powersave_bias on.
 * Returns the freq_hi to be used right now and will set freq_hi_jiffies,
 * freq_lo, and freq_lo_jiffies in percpu area for averaging freqs.
 */
static unsigned int powersave_bias_target(struct cpufreq_policy *policy,
					  unsigned int freq_next,
					  unsigned int relation)
{
	unsigned int freq_req, freq_avg;
	unsigned int freq_hi, freq_lo;
	unsigned int index = 0;
	unsigned int jiffies_total, jiffies_hi, jiffies_lo;
	int freq_reduc;
	struct cpu_dbs_info_s *dbs_info = &per_cpu(od_cpu_dbs_info,
						   policy->cpu);

	if (!dbs_info->freq_table) {
		dbs_info->freq_lo = 0;
		dbs_info->freq_lo_jiffies = 0;
		return freq_next;
	}

	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_next,
			relation, &index);
	freq_req = dbs_info->freq_table[index].frequency;
	freq_reduc = freq_req * dbs_tuners_ins.powersave_bias / 1000;
	freq_avg = freq_req - freq_reduc;

	/* Find freq bounds for freq_avg in freq_table */
	index = 0;
	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_avg,
			CPUFREQ_RELATION_H, &index);
	freq_lo = dbs_info->freq_table[index].frequency;
	index = 0;
	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_avg,
			CPUFREQ_RELATION_L, &index);
	freq_hi = dbs_info->freq_table[index].frequency;

	/* Find out how long we have to be in hi and lo freqs */
	if (freq_hi == freq_lo) {
		dbs_info->freq_lo = 0;
		dbs_info->freq_lo_jiffies = 0;
		return freq_lo;
	}
	jiffies_total = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);
	jiffies_hi = (freq_avg - freq_lo) * jiffies_total;
	jiffies_hi += ((freq_hi - freq_lo) / 2);
	jiffies_hi /= (freq_hi - freq_lo);
	jiffies_lo = jiffies_total - jiffies_hi;
	dbs_info->freq_lo = freq_lo;
	dbs_info->freq_lo_jiffies = jiffies_lo;
	dbs_info->freq_hi_jiffies = jiffies_hi;
	return freq_hi;
}

static int ondemand_powersave_bias_setspeed(struct cpufreq_policy *policy,
					    struct cpufreq_policy *altpolicy,
					    int level)
{
	if (level == POWERSAVE_BIAS_MAXLEVEL) {
		/* maximum powersave; set to lowest frequency */
		__cpufreq_driver_target(policy,
			(altpolicy) ? altpolicy->min : policy->min,
			CPUFREQ_RELATION_L);
		return 1;
	} else if (level == POWERSAVE_BIAS_MINLEVEL) {
		/* minimum powersave; set to highest frequency */
		__cpufreq_driver_target(policy,
			(altpolicy) ? altpolicy->max : policy->max,
			CPUFREQ_RELATION_H);
		return 1;
	}
	return 0;
}

static void ondemand_powersave_bias_init_cpu(int cpu)
{
	struct cpu_dbs_info_s *dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
	dbs_info->freq_table = cpufreq_frequency_get_table(cpu);
	dbs_info->freq_lo = 0;
}

static void ondemand_powersave_bias_init(void)
{
	int i;
	for_each_online_cpu(i) {
		ondemand_powersave_bias_init_cpu(i);
	}
}

void ondemand_boost_cpu(int boost)
{
	int cpu;

	if (!dbs_tuners_ins.enable_boost_cpu)
		return;

	for_each_online_cpu(cpu) {
		struct cpufreq_policy *policy;
		struct cpu_dbs_info_s *dbs_info;

		policy = cpufreq_cpu_get(cpu);
		if (!policy)
			continue;
		dbs_info = &per_cpu(od_cpu_dbs_info, policy->cpu);
		cpufreq_cpu_put(policy);

		mutex_lock(&dbs_info->timer_mutex);
		if (boost) {
			skip_ondemand = 1;
			__cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
		} else {
			skip_ondemand = 0;
		}
		mutex_unlock(&dbs_info->timer_mutex);
	}
}
EXPORT_SYMBOL(ondemand_boost_cpu);

/************************** sysfs interface ************************/

static ssize_t show_sampling_rate_min(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", min_sampling_rate);
}

define_one_global_ro(sampling_rate_min);

/* cpufreq_ondemand Governor Tunables */
#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)              \
{									\
	return sprintf(buf, "%u\n", dbs_tuners_ins.object);		\
}
show_one(sampling_rate, sampling_rate);
show_one(io_is_busy, io_is_busy);
show_one(shortcut, shortcut);
show_one(up_threshold, up_threshold);
show_one(up_threshold_multi_core, up_threshold_multi_core);
show_one(down_differential, down_differential);
show_one(sampling_down_factor, sampling_down_factor);
show_one(ignore_nice_load, ignore_nice);
show_one(optimal_freq, optimal_freq);
show_one(up_threshold_any_cpu_load, up_threshold_any_cpu_load);
show_one(sync_freq, sync_freq);
show_one(enable_boost_cpu, enable_boost_cpu)

static ssize_t show_powersave_bias
(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", dbs_tuners_ins.powersave_bias);
}

/**
 * update_sampling_rate - update sampling rate effective immediately if needed.
 * @new_rate: new sampling rate
 *
 * If new rate is smaller than the old, simply updaing
 * dbs_tuners_int.sampling_rate might not be appropriate. For example,
 * if the original sampling_rate was 1 second and the requested new sampling
 * rate is 10 ms because the user needs immediate reaction from ondemand
 * governor, but not sure if higher frequency will be required or not,
 * then, the governor may change the sampling rate too late; up to 1 second
 * later. Thus, if we are reducing the sampling rate, we need to make the
 * new value effective immediately.
 */
static void update_sampling_rate(unsigned int new_rate)
{
	int cpu;

	dbs_tuners_ins.sampling_rate = new_rate
				     = max(new_rate, min_sampling_rate);

	for_each_online_cpu(cpu) {
		struct cpufreq_policy *policy;
		struct cpu_dbs_info_s *dbs_info;
		unsigned long next_sampling, appointed_at;

		policy = cpufreq_cpu_get(cpu);
		if (!policy)
			continue;
		dbs_info = &per_cpu(od_cpu_dbs_info, policy->cpu);
		cpufreq_cpu_put(policy);

		mutex_lock(&dbs_info->timer_mutex);

		if (!delayed_work_pending(&dbs_info->work)) {
			mutex_unlock(&dbs_info->timer_mutex);
			continue;
		}

		next_sampling  = jiffies + usecs_to_jiffies(new_rate);
		appointed_at = dbs_info->work.timer.expires;

		if (time_before(next_sampling, appointed_at)) {

			mutex_unlock(&dbs_info->timer_mutex);
			cancel_delayed_work_sync(&dbs_info->work);
			mutex_lock(&dbs_info->timer_mutex);

			schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work,
						 usecs_to_jiffies(new_rate));

		}
		mutex_unlock(&dbs_info->timer_mutex);
	}
}

show_one(ui_timeout, ui_timeout);

static ssize_t store_ui_timeout(struct kobject *a, struct attribute *b,
				      const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	input = max(input, (unsigned int)DBS_UI_SAMPLING_MIN_TIMEOUT);
	dbs_tuners_ins.ui_timeout = min(input, (unsigned int)DBS_UI_SAMPLING_MAX_TIMEOUT);

	return count;
}

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_2_PHASE
//show_one(two_phase_freq, two_phase_freq);
static int two_phase_freq_array[NR_CPUS] = {[0 ... NR_CPUS-1] = 0} ;

static ssize_t show_two_phase_freq
(struct kobject *kobj, struct attribute *attr, char *buf)
{
	int i = 0 ;
	int shift = 0 ;
	char *buf_pos = buf;
	for ( i = 0 ; i < NR_CPUS; i++) {
		shift = sprintf(buf_pos,"%d,",two_phase_freq_array[i]);
		buf_pos += shift;
	}
	*(buf_pos-1) = '\0';
	return strlen(buf);
}

static ssize_t store_two_phase_freq(struct kobject *a, struct attribute *b,
		const char *buf, size_t count)
{

	int ret = 0;
	if (NR_CPUS == 1)
		ret = sscanf(buf,"%u",&two_phase_freq_array[0]);
	else if (NR_CPUS == 2)
		ret = sscanf(buf,"%u,%u",&two_phase_freq_array[0],
				&two_phase_freq_array[1]);
	else if (NR_CPUS == 4)
		ret = sscanf(buf, "%u,%u,%u,%u", &two_phase_freq_array[0],
				&two_phase_freq_array[1],
				&two_phase_freq_array[2],
				&two_phase_freq_array[3]);
	if (ret < NR_CPUS)
		return -EINVAL;

	return count;
}

#endif

static int input_event_min_freq_array[NR_CPUS] = {[0 ... NR_CPUS-1] = DBS_INPUT_EVENT_MIN_FREQ} ;

static ssize_t show_input_event_min_freq
(struct kobject *kobj, struct attribute *attr, char *buf)
{
	int i = 0 ;
	int shift = 0 ;
	char *buf_pos = buf;
	for ( i = 0 ; i < NR_CPUS; i++) {
		shift = sprintf(buf_pos,"%d,",input_event_min_freq_array[i]);
		buf_pos += shift;
	}
	*(buf_pos-1) = '\0';
	return strlen(buf);
}

static ssize_t store_input_event_min_freq(struct kobject *a, struct attribute *b,
		const char *buf, size_t count)
{

	int ret = 0;
	if (NR_CPUS == 1)
		ret = sscanf(buf,"%u",&input_event_min_freq_array[0]);
	else if (NR_CPUS == 2)
		ret = sscanf(buf,"%u,%u",&input_event_min_freq_array[0],
				&input_event_min_freq_array[1]);
	else if (NR_CPUS == 4)
		ret = sscanf(buf, "%u,%u,%u,%u", &input_event_min_freq_array[0],
				&input_event_min_freq_array[1],
				&input_event_min_freq_array[2],
				&input_event_min_freq_array[3]);
	if (ret < NR_CPUS)
		return -EINVAL;

	return count;
}

show_one(ui_sampling_rate, ui_sampling_rate);

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	if (input == dbs_tuners_ins.origin_sampling_rate)
		return count;
	update_sampling_rate(input);
	dbs_tuners_ins.origin_sampling_rate = dbs_tuners_ins.sampling_rate;
	return count;
}

static ssize_t store_ui_sampling_rate(struct kobject *a, struct attribute *b,
				      const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	dbs_tuners_ins.ui_sampling_rate = max(input, min_sampling_rate);

	return count;
}

static ssize_t store_sync_freq(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.sync_freq = input;
	return count;
}

static ssize_t store_io_is_busy(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.io_is_busy = !!input;
	return count;
}

static ssize_t store_shortcut(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (dbs_tuners_ins.shortcut != input) {
		dbs_tuners_ins.shortcut = input;
	}

	return count;
}

static ssize_t store_optimal_freq(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.optimal_freq = input;
	return count;
}

static ssize_t store_up_threshold(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_FREQUENCY_UP_THRESHOLD ||
			input < MIN_FREQUENCY_UP_THRESHOLD) {
		return -EINVAL;
	}
	dbs_tuners_ins.up_threshold = input;
	return count;
}

static ssize_t store_up_threshold_multi_core(struct kobject *a,
			struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_FREQUENCY_UP_THRESHOLD ||
			input < MIN_FREQUENCY_UP_THRESHOLD) {
		return -EINVAL;
	}
	dbs_tuners_ins.up_threshold_multi_core = input;
	return count;
}

static ssize_t store_up_threshold_any_cpu_load(struct kobject *a,
			struct attribute *b, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_FREQUENCY_UP_THRESHOLD ||
			input < MIN_FREQUENCY_UP_THRESHOLD) {
		return -EINVAL;
	}
	dbs_tuners_ins.up_threshold_any_cpu_load = input;
	return count;
}

static ssize_t store_down_differential(struct kobject *a, struct attribute *b,
		const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input >= dbs_tuners_ins.up_threshold ||
			input < MIN_FREQUENCY_DOWN_DIFFERENTIAL) {
		return -EINVAL;
	}

	dbs_tuners_ins.down_differential = input;

	return count;
}

static ssize_t store_sampling_down_factor(struct kobject *a,
			struct attribute *b, const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR || input < 1)
		return -EINVAL;
	dbs_tuners_ins.sampling_down_factor = input;

	/* Reset down sampling multiplier in case it was active */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(od_cpu_dbs_info, j);
		dbs_info->rate_mult = 1;
	}
	return count;
}

static ssize_t store_ignore_nice_load(struct kobject *a, struct attribute *b,
				      const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	unsigned int j;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == dbs_tuners_ins.ignore_nice) { /* nothing to do */
		return count;
	}
	dbs_tuners_ins.ignore_nice = input;

	/* we need to re-evaluate prev_cpu_idle */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(od_cpu_dbs_info, j);
		dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&dbs_info->prev_cpu_wall);
		if (dbs_tuners_ins.ignore_nice)
			dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];

	}
	return count;
}

static ssize_t store_powersave_bias(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	int input  = 0;
	int bypass = 0;
	int ret, cpu, reenable_timer, j;
	struct cpu_dbs_info_s *dbs_info;

	struct cpumask cpus_timer_done;
	cpumask_clear(&cpus_timer_done);

	ret = sscanf(buf, "%d", &input);

	if (ret != 1)
		return -EINVAL;

	if (input >= POWERSAVE_BIAS_MAXLEVEL) {
		input  = POWERSAVE_BIAS_MAXLEVEL;
		bypass = 1;
	} else if (input <= POWERSAVE_BIAS_MINLEVEL) {
		input  = POWERSAVE_BIAS_MINLEVEL;
		bypass = 1;
	}

	if (input == dbs_tuners_ins.powersave_bias) {
		/* no change */
		return count;
	}

	reenable_timer = ((dbs_tuners_ins.powersave_bias ==
				POWERSAVE_BIAS_MAXLEVEL) ||
				(dbs_tuners_ins.powersave_bias ==
				POWERSAVE_BIAS_MINLEVEL));

	dbs_tuners_ins.powersave_bias = input;
	if (!bypass) {
		if (reenable_timer) {
			/* reinstate dbs timer */
			for_each_online_cpu(cpu) {
				if (lock_policy_rwsem_write(cpu) < 0)
					continue;

				dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

				for_each_cpu(j, &cpus_timer_done) {
					if (!dbs_info->cur_policy) {
						pr_err("Dbs policy is NULL\n");
						goto skip_this_cpu;
					}
					if (cpumask_test_cpu(j, dbs_info->
							cur_policy->cpus))
						goto skip_this_cpu;
				}

				cpumask_set_cpu(cpu, &cpus_timer_done);
				if (dbs_info->cur_policy) {
					/* restart dbs timer */
					dbs_timer_init(dbs_info);
				}
skip_this_cpu:
				unlock_policy_rwsem_write(cpu);
			}
		}
		ondemand_powersave_bias_init();
	} else {
		/* running at maximum or minimum frequencies; cancel
		   dbs timer as periodic load sampling is not necessary */
		for_each_online_cpu(cpu) {
			if (lock_policy_rwsem_write(cpu) < 0)
				continue;

			dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

			for_each_cpu(j, &cpus_timer_done) {
				if (!dbs_info->cur_policy) {
					pr_err("Dbs policy is NULL\n");
					goto skip_this_cpu_bypass;
				}
				if (cpumask_test_cpu(j, dbs_info->
							cur_policy->cpus))
					goto skip_this_cpu_bypass;
			}

			cpumask_set_cpu(cpu, &cpus_timer_done);

			if (dbs_info->cur_policy) {
				/* cpu using ondemand, cancel dbs timer */
				mutex_lock(&dbs_info->timer_mutex);
				dbs_timer_exit(dbs_info);

				ondemand_powersave_bias_setspeed(
					dbs_info->cur_policy,
					NULL,
					input);

				mutex_unlock(&dbs_info->timer_mutex);
			}
skip_this_cpu_bypass:
			unlock_policy_rwsem_write(cpu);
		}
	}

	return count;
}

static ssize_t store_enable_boost_cpu(struct kobject *a, struct attribute *b,
				const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if(ret != 1)
		return -EINVAL;

	dbs_tuners_ins.enable_boost_cpu = (input > 0 ? input : 0);
#ifdef CONFIG_EARLYSUSPEND_BOOST_CPU_SPEED
	has_boost_cpu_func = (unsigned int) dbs_tuners_ins.enable_boost_cpu;
#endif
	return count;
}

define_one_global_rw(sampling_rate);
define_one_global_rw(io_is_busy);
define_one_global_rw(shortcut);
define_one_global_rw(up_threshold);
define_one_global_rw(down_differential);
define_one_global_rw(sampling_down_factor);
define_one_global_rw(ignore_nice_load);
define_one_global_rw(powersave_bias);
define_one_global_rw(up_threshold_multi_core);
define_one_global_rw(optimal_freq);
define_one_global_rw(up_threshold_any_cpu_load);
define_one_global_rw(sync_freq);
#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_2_PHASE
define_one_global_rw(two_phase_freq);
#endif
define_one_global_rw(input_event_min_freq);
define_one_global_rw(ui_sampling_rate);
define_one_global_rw(ui_timeout);
define_one_global_rw(enable_boost_cpu);

static struct attribute *dbs_attributes[] = {
	&sampling_rate_min.attr,
	&sampling_rate.attr,
	&up_threshold.attr,
	&down_differential.attr,
	&sampling_down_factor.attr,
	&ignore_nice_load.attr,
	&powersave_bias.attr,
	&io_is_busy.attr,
	&shortcut.attr,
	&up_threshold_multi_core.attr,
	&optimal_freq.attr,
	&up_threshold_any_cpu_load.attr,
	&sync_freq.attr,
#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_2_PHASE
	&two_phase_freq.attr,
#endif
	&input_event_min_freq.attr,
	&ui_sampling_rate.attr,
	&ui_timeout.attr,
	&enable_boost_cpu.attr,
	NULL
};

static struct attribute_group dbs_attr_group = {
	.attrs = dbs_attributes,
	.name = "ondemand",
};

/************************** sysfs end ************************/

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_MULTI_PHASE
static inline void switch_turbo_mode(unsigned timeout)
{
	if (timeout > 0)
		mod_timer(&freq_mode_timer, jiffies + msecs_to_jiffies(timeout));

	tbl_select[0] = 2;
	tbl_select[1] = 3;
	tbl_select[2] = 4;
	tbl_select[3] = 4;
}

static inline void switch_normal_mode(void)
{
	if (input_event_counter > 0)
		return;

	tbl_select[0] = 0;
	tbl_select[1] = 1;
	tbl_select[2] = 2;
	tbl_select[3] = 3;
}

static void switch_mode_timer(unsigned long data)
{
	switch_normal_mode();
}

static void dbs_init_freq_map_table(struct cpufreq_policy *policy)
{
	unsigned int min_diff, top1, top2;
	int cnt, i, j;

	tbl = cpufreq_frequency_get_table(0);
	min_diff = policy->cpuinfo.max_freq;

	/* find the minimum differential gap */
	for (cnt = 0; (tbl[cnt].frequency != CPUFREQ_TABLE_END); cnt++) {
		if (cnt > 0)
			min_diff = MIN(tbl[cnt].frequency - tbl[cnt-1].frequency, min_diff);
	}

	/* measure the top frequency in each step */
	top1 = (policy->cpuinfo.max_freq + policy->cpuinfo.min_freq) / 2;
	top2 = (policy->cpuinfo.max_freq + top1) / 2;

	for (i = 0; i < TABLE_SIZE; i++) {
		/* allocate the buffer */
		tblmap[i] = kmalloc(sizeof(unsigned int) * cnt, GFP_KERNEL);
		BUG_ON(!tblmap[i]);
		/* copy original frequency */
		for (j = 0; j < cnt; j++)
			tblmap[i][j] = tbl[j].frequency;
	}

	for (j = 0; j < cnt; j++) {
		/* consider top1 boundary */
		if (tbl[j].frequency < top1) {
			tblmap[0][j] += MAX((top1 - tbl[j].frequency)/3, min_diff);
		}
		/* consider top2 boundary */
		if (tbl[j].frequency < top2) {
			tblmap[1][j] += MAX((top2 - tbl[j].frequency)/3, min_diff);
			tblmap[2][j] += MAX(((top2 - tbl[j].frequency)*2)/5, min_diff);
			tblmap[3][j] += MAX((top2 - tbl[j].frequency)/2, min_diff);
		}
		else {
			tblmap[3][j] += MAX((policy->cpuinfo.max_freq - tbl[j].frequency)/3, min_diff);
		}
		/* for all conditions */
		tblmap[4][j] += MAX((policy->cpuinfo.max_freq - tbl[j].frequency)/2, min_diff);
	}

	switch_normal_mode();

	/* initial freq table switch timer */
	init_timer(&freq_mode_timer);
	freq_mode_timer.function = switch_mode_timer;
	freq_mode_timer.data = 0;

#if 0
	/* dump the mapping table */
	for (i = 0; i < TABLE_SIZE; i++) {
		pr_info("Table %d shows:\n", i+1);
		for (j = 0; j < cnt; j++) {
			pr_info("%02d: %8u\n", j, tblmap[i][j]);
		}
	}
#endif
}

static void dbs_deinit_freq_map_table(void)
{
	int i;

	if (!tbl)
		return;

	tbl = NULL;

	for (i = 0; i < TABLE_SIZE; i++)
		kfree(tblmap[i]);

	del_timer(&freq_mode_timer);
}

static inline int get_cpu_freq_index(unsigned int freq)
{
	static int saved_index = 0;
	int index;

	if (!tbl) {
		pr_warn("tbl is NULL, use previous value %d\n", saved_index);
		return saved_index;
	}

	for (index = 0; (tbl[index].frequency != CPUFREQ_TABLE_END); index++) {
		if (tbl[index].frequency >= freq) {
			saved_index = index;
			break;
		}
	}

	return index;
}
#endif

static void dbs_freq_increase(struct cpufreq_policy *p, unsigned load, unsigned int freq)
{
	if (dbs_tuners_ins.powersave_bias)
		freq = powersave_bias_target(p, freq, CPUFREQ_RELATION_H);
	else if (p->cur == p->max) {
		trace_cpufreq_interactive_already (p->cpu, load, p->cur, p->cur);
		return;
	}

	trace_cpufreq_interactive_target (p->cpu, load, p->cur, freq);

	__cpufreq_driver_target(p, freq, (dbs_tuners_ins.powersave_bias || freq < p->max) ?
			CPUFREQ_RELATION_L : CPUFREQ_RELATION_H);

	trace_cpufreq_interactive_up (p->cpu, freq, p->cur);
}

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_2_PHASE
int set_two_phase_freq(int cpufreq)
{
	int i  = 0;
	for ( i = 0 ; i < NR_CPUS; i++)
		two_phase_freq_array[i] = cpufreq;
	return 0;
}

void set_two_phase_freq_by_cpu ( int cpu_nr, int cpufreq){
	two_phase_freq_array[cpu_nr-1] = cpufreq;
}
#endif

int input_event_boosted(void)
{
	unsigned long flags;

	/* Do not decrease freq unless input event sampling period is expired */
	spin_lock_irqsave(&input_boost_lock, flags);
	if (input_event_boost) {
		if (time_before(jiffies, input_event_boost_expired)) {
			spin_unlock_irqrestore(&input_boost_lock, flags);
			return 1;
		}
		input_event_boost = false;
		dbs_tuners_ins.sampling_rate = dbs_tuners_ins.origin_sampling_rate;
	}
	spin_unlock_irqrestore(&input_boost_lock, flags);

	return 0;
}

static unsigned int get_cpu_current_load(unsigned int j, unsigned int *record)
{
	/* Current load across this CPU */
	unsigned int cur_load = 0;
	struct cpu_dbs_info_s *j_dbs_info;
	cputime64_t cur_wall_time, cur_idle_time, cur_iowait_time;
	unsigned int idle_time, wall_time, iowait_time;

	j_dbs_info = &per_cpu(od_cpu_dbs_info, j);

	if (record)
		*record = j_dbs_info->prev_load;

	cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);
	cur_iowait_time = get_cpu_iowait_time(j, &cur_wall_time);

	wall_time = (unsigned int)
		(cur_wall_time - j_dbs_info->prev_cpu_wall);
	j_dbs_info->prev_cpu_wall = cur_wall_time;

	idle_time = (unsigned int)
		(cur_idle_time - j_dbs_info->prev_cpu_idle);
	j_dbs_info->prev_cpu_idle = cur_idle_time;

	iowait_time = (unsigned int)
		(cur_iowait_time - j_dbs_info->prev_cpu_iowait);
	j_dbs_info->prev_cpu_iowait = cur_iowait_time;

	if (dbs_tuners_ins.ignore_nice) {
		u64 cur_nice;
		unsigned long cur_nice_jiffies;

		cur_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE] -
				 j_dbs_info->prev_cpu_nice;
		/*
		 * Assumption: nice time between sampling periods will
		 * be less than 2^32 jiffies for 32 bit sys
		 */
		cur_nice_jiffies = (unsigned long)
				cputime64_to_jiffies64(cur_nice);

		j_dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];
		idle_time += jiffies_to_usecs(cur_nice_jiffies);
	}

	/*
	 * For the purpose of ondemand, waiting for disk IO is an
	 * indication that you're performance critical, and not that
	 * the system is actually idle. So subtract the iowait time
	 * from the cpu idle time.
	 */

	if (dbs_tuners_ins.io_is_busy && idle_time >= iowait_time)
		idle_time -= iowait_time;

	if (unlikely(!wall_time || wall_time < idle_time))
		return j_dbs_info->prev_load;

	cur_load = 100 * (wall_time - idle_time) / wall_time;
	j_dbs_info->max_load  = max(cur_load, j_dbs_info->prev_load);
	j_dbs_info->prev_load = cur_load;

	return cur_load;
}

static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
	/* Extrapolated load of this CPU */
	unsigned int load_at_max_freq = 0;
	unsigned int max_load_freq;
	/* Current load across this CPU */
	unsigned int cur_load = 0;

	unsigned int max_load_other_cpu = 0;
	struct cpufreq_policy *policy;
	unsigned int j, prev_load = 0, freq_next;
#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_2_PHASE
	static unsigned int phase = 0;
	static unsigned int counter = 0;
	unsigned int nr_cpus;
#endif
	unsigned int up_threshold = dbs_tuners_ins.up_threshold;;

	this_dbs_info->freq_lo = 0;
	policy = this_dbs_info->cur_policy;

	/*
	 * Every sampling_rate, we check, if current idle time is less
	 * than 20% (default), then we try to increase frequency
	 * Every sampling_rate, we look for a the lowest
	 * frequency which can sustain the load while keeping idle time over
	 * 30%. If such a frequency exist, we try to decrease to this frequency.
	 *
	 * Any frequency increase takes it to the maximum frequency.
	 * Frequency reduction happens at minimum steps of
	 * 5% (default) of current frequency
	 */

	/* Get Absolute Load - in terms of freq */
	max_load_freq = 0;

	for_each_cpu(j, policy->cpus) {
		unsigned int load_freq;
		int freq_avg;

		cur_load = get_cpu_current_load(j, &prev_load);
		freq_avg = __cpufreq_driver_getavg(policy, j);
		if (freq_avg <= 0)
			freq_avg = policy->cur;

		load_freq = cur_load * freq_avg;
		if (load_freq > max_load_freq)
			max_load_freq = load_freq;

		/* calculate the scaled load across CPU */
		load_at_max_freq += (cur_load * policy->cur) /
					policy->cpuinfo.max_freq;
	}

	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *j_dbs_info;
		j_dbs_info = &per_cpu(od_cpu_dbs_info, j);

		if (j == policy->cpu)
			continue;

		if (max_load_other_cpu < j_dbs_info->max_load)
			max_load_other_cpu = j_dbs_info->max_load;
		/*
		 * The other cpu could be running at higher frequency
		 * but may not have completed it's sampling_down_factor.
		 * For that case consider other cpu is loaded so that
		 * frequency imbalance does not occur.
		 */

		if ((j_dbs_info->cur_policy != NULL)
			&& (j_dbs_info->cur_policy->cur ==
					j_dbs_info->cur_policy->max)) {

			if (policy->cur >= dbs_tuners_ins.optimal_freq)
				max_load_other_cpu =
				dbs_tuners_ins.up_threshold_any_cpu_load;
		}
	}

	cpufreq_notify_utilization(policy, load_at_max_freq);

	/* Check for frequency increase */
#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_MULTI_PHASE

	if (!dbs_tuners_ins.shortcut)
		up_threshold = up_threshold_level[1];

	if (max_load_freq > up_threshold * policy->cur) {
		unsigned int avg_load;
		int index;

		/* force to set max freq */
		if (dbs_tuners_ins.shortcut) {
			freq_next = policy->cpuinfo.max_freq;
			goto set_freq;
		}

		avg_load = (prev_load + cur_load) >> 1;
		index = get_cpu_freq_index(policy->cur);

		/* For suddenly burst case */
		if (FREQ_NEED_BURST(policy->cur) && cur_load > up_threshold_level[0]) {
			freq_next = tblmap[tbl_select[3]][index];
		}
		/* apply highest performance table due to average loading is heavy */
		else if (avg_load > up_threshold_level[0]) {
			freq_next = tblmap[tbl_select[3]][index];
		}
		/* apply low performance table due to average loading is not high */
		else if (avg_load <= up_threshold_level[1]) {
			freq_next = tblmap[tbl_select[0]][index];
		}
		/* other case: average loading is high but not heavy */
		else {
			/* apply high performance table due to current loading is heavy */
			if (cur_load > up_threshold_level[0]) {
				freq_next = tblmap[tbl_select[2]][index];
			}
			/* apply medium performance table due to current loading is high but not heavy */
			else {
				freq_next = tblmap[tbl_select[1]][index];
			}
		}

set_freq:
		dbs_freq_increase(policy, cur_load, freq_next);
		/* If switching to max speed, apply sampling_down_factor */
		if (policy->cur == policy->max)
			this_dbs_info->rate_mult = dbs_tuners_ins.sampling_down_factor;
		return;
	}
#else
	if (max_load_freq > up_threshold * policy->cur) {
		/* If switching to max speed, apply sampling_down_factor */
#ifndef CONFIG_CPU_FREQ_GOV_ONDEMAND_2_PHASE
		if (policy->cur < policy->max)
			this_dbs_info->rate_mult =
				dbs_tuners_ins.sampling_down_factor;
		dbs_freq_increase(policy, cur_load, policy->max);
#else
		/* force to set max freq */
		if (dbs_tuners_ins.shortcut)
			counter = 2;

		if (counter < 5) {
			counter++;
			if (counter > 2) {
				/* change to busy phase */
				phase = 1;
			}
		}

		nr_cpus = num_online_cpus();
		dbs_tuners_ins.two_phase_freq = two_phase_freq_array[nr_cpus-1];
		if (dbs_tuners_ins.two_phase_freq < policy->cur)
			phase=1;

		if (dbs_tuners_ins.two_phase_freq != 0 && phase == 0) {
			/* idle phase */
			dbs_freq_increase(policy, cur_load, dbs_tuners_ins.two_phase_freq);
		} else {
			/* busy phase */
			if (policy->cur < policy->max)
				this_dbs_info->rate_mult =
					dbs_tuners_ins.sampling_down_factor;
			dbs_freq_increase(policy, cur_load, policy->max);
		}
#endif
		return;
	}
#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_2_PHASE
	if (counter > 0) {
		counter--;
		if (counter == 0) {
			/* change to idle phase */
			phase = 0;
		}
	}
#endif
#endif

	if (num_online_cpus() > 1) {
		if (max_load_other_cpu >
				dbs_tuners_ins.up_threshold_any_cpu_load) {
			if (policy->cur < dbs_tuners_ins.sync_freq)
				dbs_freq_increase(policy, cur_load,
						dbs_tuners_ins.sync_freq);
			else
				trace_cpufreq_interactive_already (policy->cpu, cur_load, policy->cur,policy->cur);
			return;
		}

		if (max_load_freq > dbs_tuners_ins.up_threshold_multi_core *
								policy->cur) {
			if (policy->cur < dbs_tuners_ins.optimal_freq)
				dbs_freq_increase(policy, cur_load,
						dbs_tuners_ins.optimal_freq);
			else
				trace_cpufreq_interactive_already (policy->cpu, cur_load, policy->cur,policy->cur);
			return;
		}
	}

	if (input_event_boosted())
	{
		trace_cpufreq_interactive_already (policy->cpu, cur_load, policy->cur,policy->cur);
		return;
	}

	/* Check for frequency decrease */
	/* if we cannot reduce the frequency anymore, break out early */
	if (policy->cur == policy->min){
		trace_cpufreq_interactive_already (policy->cpu, cur_load, policy->cur,policy->cur);
		return;
	}


	/*
	 * The optimal frequency is the frequency that is the lowest that
	 * can support the current CPU usage without triggering the up
	 * policy. To be safe, we focus 10 points under the threshold.
	 */
	if (max_load_freq <
	    (dbs_tuners_ins.up_threshold - dbs_tuners_ins.down_differential) *
	     policy->cur) {
		freq_next = max_load_freq /
				(dbs_tuners_ins.up_threshold -
				 dbs_tuners_ins.down_differential);

		/* No longer fully busy, reset rate_mult */
		this_dbs_info->rate_mult = 1;

		if (freq_next < policy->min)
			freq_next = policy->min;

		if (num_online_cpus() > 1) {
			if (max_load_other_cpu >
			(dbs_tuners_ins.up_threshold_multi_core -
			dbs_tuners_ins.down_differential) &&
			freq_next < dbs_tuners_ins.sync_freq)
				freq_next = dbs_tuners_ins.sync_freq;

			if (dbs_tuners_ins.optimal_freq > policy->min && max_load_freq >
				 (dbs_tuners_ins.up_threshold_multi_core -
				  dbs_tuners_ins.down_differential_multi_core) *
				  policy->cur)
				freq_next = dbs_tuners_ins.optimal_freq;

		}

		if (dbs_tuners_ins.powersave_bias)
			freq_next = powersave_bias_target(policy, freq_next, CPUFREQ_RELATION_L);

		trace_cpufreq_interactive_target (policy->cpu, cur_load, policy->cur, freq_next);
		__cpufreq_driver_target(policy, freq_next,
			CPUFREQ_RELATION_L);
		trace_cpufreq_interactive_down (policy->cpu, freq_next, policy->cur);
	}
}

static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, work.work);
	unsigned int cpu = dbs_info->cpu;
	int sample_type = dbs_info->sample_type;
	int delay = msecs_to_jiffies(50);

	mutex_lock(&dbs_info->timer_mutex);

	if (skip_ondemand)
		goto sched_wait;

	/* Common NORMAL_SAMPLE setup */
	dbs_info->sample_type = DBS_NORMAL_SAMPLE;
	if (!dbs_tuners_ins.powersave_bias ||
	    sample_type == DBS_NORMAL_SAMPLE) {
		dbs_check_cpu(dbs_info);
		if (dbs_info->freq_lo) {
			/* Setup timer for SUB_SAMPLE */
			dbs_info->sample_type = DBS_SUB_SAMPLE;
			delay = dbs_info->freq_hi_jiffies;
		} else {
			/* We want all CPUs to do sampling nearly on
			 * same jiffy
			 */
			delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate
				* dbs_info->rate_mult);

			if (num_online_cpus() > 1)
				delay -= jiffies % delay;
		}
	} else {
		if (input_event_boosted())
			goto sched_wait;

		__cpufreq_driver_target(dbs_info->cur_policy,
			dbs_info->freq_lo, CPUFREQ_RELATION_H);
		delay = dbs_info->freq_lo_jiffies;
	}

sched_wait:
	schedule_delayed_work_on(cpu, &dbs_info->work, delay);
	mutex_unlock(&dbs_info->timer_mutex);
}

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);

	if (num_online_cpus() > 1)
		delay -= jiffies % delay;

	dbs_info->sample_type = DBS_NORMAL_SAMPLE;
	INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
	schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work, delay);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
	cancel_delayed_work_sync(&dbs_info->work);
}

/*
 * Not all CPUs want IO time to be accounted as busy; this dependson how
 * efficient idling at a higher frequency/voltage is.
 * Pavel Machek says this is not so for various generations of AMD and old
 * Intel systems.
 * Mike Chan (androidlcom) calis this is also not true for ARM.
 * Because of this, whitelist specific known (series) of CPUs by default, and
 * leave all others up to the user.
 */
static int should_io_be_busy(void)
{
#if defined(CONFIG_X86)
	/*
	 * For Intel, Core 2 (model 15) andl later have an efficient idle.
	 */
	if (boot_cpu_data.x86_vendor == X86_VENDOR_INTEL &&
	    boot_cpu_data.x86 == 6 &&
	    boot_cpu_data.x86_model >= 15)
		return 1;
#endif
	return 0;
}


static void dbs_input_event(struct input_handle *handle, unsigned int type,
		unsigned int code, int value)
{
	int i;
	struct cpu_dbs_info_s *dbs_info;
	unsigned long flags;
	int input_event_min_freq;

	if ((dbs_tuners_ins.powersave_bias == POWERSAVE_BIAS_MAXLEVEL) ||
		(dbs_tuners_ins.powersave_bias == POWERSAVE_BIAS_MINLEVEL)) {
		/* nothing to do */
		return;
	}

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_MULTI_PHASE
	if (type == EV_SYN && code == SYN_REPORT) {
		/* Reset powersave_bias. */
		dbs_tuners_ins.powersave_bias = 0;
	}
	else if (type == EV_ABS && code == ABS_MT_TRACKING_ID) {

		if (value != -1) {		/* Touch Down */

			input_event_min_freq = input_event_min_freq_array[num_online_cpus() - 1];

			input_event_counter++;
			switch_turbo_mode(0);

			/* (Re)set input event sampling expired time */
			spin_lock_irqsave(&input_boost_lock, flags);
			input_event_boost = true;
			input_event_boost_expired = jiffies + usecs_to_jiffies(dbs_tuners_ins.sampling_rate * 4);
			spin_unlock_irqrestore(&input_boost_lock, flags);

			for_each_online_cpu(i) {
				dbs_info = &per_cpu(od_cpu_dbs_info, i);
				/* Increase frequency to input_event_min_freq at touch down. */
				if (dbs_info->cur_policy
					&& dbs_info->cur_policy->cur < input_event_min_freq) {
					dbs_info->input_event_freq = input_event_min_freq;
					wake_up_process(per_cpu(up_task, i));
				}
			}
		}
		else {		/* Touch Up */
			if (likely(input_event_counter > 0))
				input_event_counter--;
			else
				pr_warning("dbs_input_event: Touch isn't paired!\n");

			/* Stay in turbo mode for a while. */
			switch_turbo_mode(DBS_SWITCH_MODE_TIMEOUT);
		}
	}
#else
	if (type == EV_SYN && code == SYN_REPORT) {

		/* (Re)set input event sampling expired time */
		spin_lock_irqsave(&input_boost_lock, flags);
		input_event_boost = true;
		input_event_boost_expired = jiffies + msecs_to_jiffies(dbs_tuners_ins.ui_timeout);
		spin_unlock_irqrestore(&input_boost_lock, flags);

		input_event_min_freq = input_event_min_freq_array[num_online_cpus() - 1];
		for_each_online_cpu(i) {
			/* Prefer cpufreq initialization has finished */
			if (likely(per_cpu(cpufreq_init_done, i))) {
				dbs_info = &per_cpu(od_cpu_dbs_info, i);
				if (dbs_info->cur_policy &&		/* Check for not NULL */
					dbs_info->cur_policy->cur < input_event_min_freq) {
					dbs_info->input_event_freq = input_event_min_freq;
					wake_up_process(per_cpu(up_task, i));
				}
			} else {
				pr_info("dbs_input_event: cpu%d not init done...\n", i);
			}
		}
	}
#endif
}

static int input_dev_filter(const char *input_dev_name)
{
	if (strstr(input_dev_name, "touchscreen") ||
	    strstr(input_dev_name, "keypad")) {
		return 0; /* inputs we are interested */
	} else {
		return 1;
	}
}

static int dbs_input_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	/* filter out those input_dev that we don't care */
	if (input_dev_filter(dev->name))
		return -ENODEV;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "cpufreq";

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void dbs_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id dbs_ids[] = {
	{ .driver_info = 1 },
	{ },
};

static struct input_handler dbs_input_handler = {
	.event		= dbs_input_event,
	.connect	= dbs_input_connect,
	.disconnect	= dbs_input_disconnect,
	.name		= "cpufreq_ond",
	.id_table	= dbs_ids,
};

int set_input_event_min_freq(int cpufreq)
{
	int i  = 0;
	for ( i = 0 ; i < NR_CPUS; i++)
		input_event_min_freq_array[i] = cpufreq;
	return 0;
}

void set_input_event_min_freq_by_cpu ( int cpu_nr, int cpufreq){
	input_event_min_freq_array[cpu_nr-1] = cpufreq;
}
static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int j;
	int rc;

	this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
		if ((!cpu_online(cpu)) || (!policy->cur))
			return -EINVAL;

		mutex_lock(&dbs_mutex);

		dbs_enable++;
		for_each_cpu(j, policy->cpus) {
			struct cpu_dbs_info_s *j_dbs_info;
			j_dbs_info = &per_cpu(od_cpu_dbs_info, j);
			j_dbs_info->cur_policy = policy;

			j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&j_dbs_info->prev_cpu_wall);
			if (dbs_tuners_ins.ignore_nice)
				j_dbs_info->prev_cpu_nice =
						kcpustat_cpu(j).cpustat[CPUTIME_NICE];
		}
		this_dbs_info->cpu = cpu;
		this_dbs_info->rate_mult = 1;
		ondemand_powersave_bias_init_cpu(cpu);
		/*
		 * Start the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 1) {
			unsigned int latency;

			rc = sysfs_create_group(cpufreq_global_kobject,
						&dbs_attr_group);
			if (rc) {
				mutex_unlock(&dbs_mutex);
				return rc;
			}

			/* policy latency is in nS. Convert it to uS first */
			latency = policy->cpuinfo.transition_latency / 1000;
			if (latency == 0)
				latency = 1;
			/* Bring kernel and HW constraints together */
			min_sampling_rate = max(min_sampling_rate,
					MIN_LATENCY_MULTIPLIER * latency);
			dbs_tuners_ins.sampling_rate =
				max(min_sampling_rate,
				    latency * LATENCY_MULTIPLIER);
			if (dbs_tuners_ins.sampling_rate < DEF_SAMPLING_RATE)
				dbs_tuners_ins.sampling_rate = DEF_SAMPLING_RATE;
			dbs_tuners_ins.origin_sampling_rate = dbs_tuners_ins.sampling_rate;
			dbs_tuners_ins.io_is_busy = should_io_be_busy();

			if (dbs_tuners_ins.optimal_freq == 0)
				dbs_tuners_ins.optimal_freq = policy->min;

			if (dbs_tuners_ins.sync_freq == 0)
				dbs_tuners_ins.sync_freq = policy->min;

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_MULTI_PHASE
			dbs_init_freq_map_table(policy);
#endif
		}
		if (!cpu)
			rc = input_register_handler(&dbs_input_handler);
		mutex_unlock(&dbs_mutex);

		mutex_init(&this_dbs_info->timer_mutex);

		if (!ondemand_powersave_bias_setspeed(
					this_dbs_info->cur_policy,
					NULL,
					dbs_tuners_ins.powersave_bias))
			dbs_timer_init(this_dbs_info);
		trace_cpufreq_interactive_target (cpu, 0, 0, 0);
		break;

	case CPUFREQ_GOV_STOP:
		dbs_timer_exit(this_dbs_info);

		mutex_lock(&dbs_mutex);
//		mutex_destroy(&this_dbs_info->timer_mutex);
		dbs_enable--;
		/* If device is being removed, policy is no longer
		 * valid. */
		this_dbs_info->cur_policy = NULL;
		if (!cpu)
			input_unregister_handler(&dbs_input_handler);
		mutex_unlock(&dbs_mutex);
		if (!dbs_enable) {
#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_MULTI_PHASE
			dbs_deinit_freq_map_table();
#endif
			sysfs_remove_group(cpufreq_global_kobject,
					   &dbs_attr_group);
		}
		trace_cpufreq_interactive_target (cpu, 0, 0, 0);
		break;

	case CPUFREQ_GOV_LIMITS:
		mutex_lock(&this_dbs_info->timer_mutex);
		if(this_dbs_info->cur_policy){
			if (policy->max < this_dbs_info->cur_policy->cur)
				__cpufreq_driver_target(this_dbs_info->cur_policy,
					policy->max, CPUFREQ_RELATION_H);
			else if (policy->min > this_dbs_info->cur_policy->cur)
				__cpufreq_driver_target(this_dbs_info->cur_policy,
					policy->min, CPUFREQ_RELATION_L);
			else if (dbs_tuners_ins.powersave_bias != 0)
				ondemand_powersave_bias_setspeed(
					this_dbs_info->cur_policy,
					policy,
					dbs_tuners_ins.powersave_bias);
		}
		mutex_unlock(&this_dbs_info->timer_mutex);
		break;
	}
	return 0;
}

static int cpufreq_gov_dbs_up_task(void *data)
{
	struct cpufreq_policy *policy;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int cpu = smp_processor_id();

	while (1) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();

		if (kthread_should_stop())
			break;

		set_current_state(TASK_RUNNING);

		get_online_cpus();

		if (lock_policy_rwsem_write(cpu) < 0)
			goto bail_acq_sema_failed;

		this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
		policy = this_dbs_info->cur_policy;
		if (!policy) {
			/* CPU not using ondemand governor */
			goto bail_incorrect_governor;
		}

		mutex_lock(&this_dbs_info->timer_mutex);

		/* Clear powersave_bias because we don't want the freq to be decreased. */
		dbs_tuners_ins.powersave_bias = 0;
		dbs_freq_increase(policy, this_dbs_info->prev_load, this_dbs_info->input_event_freq);
		this_dbs_info->prev_cpu_idle = get_cpu_idle_time(cpu, &this_dbs_info->prev_cpu_wall);

		mutex_unlock(&this_dbs_info->timer_mutex);

bail_incorrect_governor:
		unlock_policy_rwsem_write(cpu);

bail_acq_sema_failed:
		put_online_cpus();

#ifndef CONFIG_CPU_FREQ_GOV_ONDEMAND_MULTI_PHASE
		dbs_tuners_ins.sampling_rate = dbs_tuners_ins.ui_sampling_rate;
#endif
	}

	return 0;
}

static int __init cpufreq_gov_dbs_init(void)
{
	u64 idle_time;
	unsigned int i;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO-1 };
	struct task_struct *pthread;
	int cpu = get_cpu();

	idle_time = get_cpu_idle_time_us(cpu, NULL);
	put_cpu();
	if (idle_time != -1ULL) {
		/* Idle micro accounting is supported. Use finer thresholds */
		dbs_tuners_ins.up_threshold = MICRO_FREQUENCY_UP_THRESHOLD;
		dbs_tuners_ins.down_differential =
					MICRO_FREQUENCY_DOWN_DIFFERENTIAL;
		/*
		 * In nohz/micro accounting case we set the minimum frequency
		 * not depending on HZ, but fixed (very low). The deferred
		 * timer might skip some samples if idle/sleeping as needed.
		*/
		min_sampling_rate = MICRO_FREQUENCY_MIN_SAMPLE_RATE;
	} else {
		/* For correct statistics, we need 10 ticks for each measure */
		min_sampling_rate =
			MIN_SAMPLING_RATE_RATIO * jiffies_to_usecs(10);
	}

#ifdef CONFIG_EARLYSUSPEND_BOOST_CPU_SPEED
	has_boost_cpu_func = (unsigned int) dbs_tuners_ins.enable_boost_cpu;
#endif

	spin_lock_init(&input_boost_lock);

	for_each_possible_cpu(i) {
		pthread = kthread_create_on_node(cpufreq_gov_dbs_up_task,
								NULL, cpu_to_node(i),
								"kdbs_up/%d", i);
		if (likely(!IS_ERR(pthread))) {
			kthread_bind(pthread, i);
			sched_setscheduler_nocheck(pthread, SCHED_FIFO, &param);
			get_task_struct(pthread);
			per_cpu(up_task, i) = pthread;
		}
	}
	return cpufreq_register_governor(&cpufreq_gov_ondemand);
}

static void __exit cpufreq_gov_dbs_exit(void)
{
	unsigned int i;

	cpufreq_unregister_governor(&cpufreq_gov_ondemand);
	for_each_possible_cpu(i) {
		struct cpu_dbs_info_s *this_dbs_info =
			&per_cpu(od_cpu_dbs_info, i);
		mutex_destroy(&this_dbs_info->timer_mutex);
		if (per_cpu(up_task, i)) {
			kthread_stop(per_cpu(up_task, i));
			put_task_struct(per_cpu(up_task, i));
		}
	}
}


MODULE_AUTHOR("Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>");
MODULE_AUTHOR("Alexey Starikovskiy <alexey.y.starikovskiy@intel.com>");
MODULE_DESCRIPTION("'cpufreq_ondemand' - A dynamic cpufreq governor for "
	"Low Latency Frequency Transition capable processors");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMAND
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
