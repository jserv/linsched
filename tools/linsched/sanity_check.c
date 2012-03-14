
#include "linsched.h"
#include "sanity_check.h"

static unsigned long check_cfs_rq(struct cfs_rq *cfs_rq);

static unsigned long check_cfs_se(struct sched_entity *se, struct cfs_rq *cfs_rq)
{
	BUG_ON(se->cfs_rq != cfs_rq);
	BUG_ON(!se->on_rq);

	if (se->my_q) {
		unsigned long tasks = check_cfs_rq(se->my_q);
		BUG_ON(!tasks);
		return tasks;
	} else {
		struct task_struct *p = container_of(se, struct task_struct, se);
		BUG_ON(cpu_rq(task_cpu(p)) != cfs_rq->rq);
		BUG_ON(task_cpu(p) != debug_smp_processor_id());
	}
	return 1;
}

static unsigned long check_cfs_rq(struct cfs_rq *cfs_rq)
{
	unsigned long h_nr_running = 0;
	unsigned long nr_running = 0;
	u64 last_vruntime = 0;
	unsigned long load = 0;
	struct rb_node *node, *first;

	if (cfs_rq->curr) {
		h_nr_running += check_cfs_se(cfs_rq->curr, cfs_rq);
		nr_running++;
		load += cfs_rq->curr->load.weight;
	}

	first = cfs_rq->rb_leftmost;
	BUG_ON(first != rb_first(&cfs_rq->tasks_timeline));

	for(node = first; node; node = rb_next(node)) {
		struct sched_entity *se;
		se = rb_entry(node, struct sched_entity, run_node);

		BUG_ON(node != first &&
		       (s64)(se->vruntime - last_vruntime) < 0);
		last_vruntime = se->vruntime;
		load += se->load.weight;

		h_nr_running += check_cfs_se(se, cfs_rq);
		nr_running++;
	}
	BUG_ON(load != cfs_rq->load.weight);
	BUG_ON(nr_running != cfs_rq->nr_running);

	return h_nr_running;
}

static unsigned long check_rt_rq(struct rt_rq *rt_rq);

static unsigned long check_rt_se(struct sched_rt_entity *rt_se, struct rt_rq *rt_rq)
{
#ifdef CONFIG_RT_GROUP_SCHED
	BUG_ON(rt_se->parent != rt_rq);
	if (rt_se->my_q) {
		unsigned long tasks = check_rt_rq(rt_se->my_q);;
		BUG_ON(!tasks);
		return tasks;
	}
#endif
	return 1;
}

static unsigned long check_rt_rq(struct rt_rq *rt_rq)
{
	unsigned long nr_running = 0;
	unsigned long h_nr_running = 0;
	struct rt_prio_array *array = &rt_rq->active;
	int idx = -1;

	while((idx = find_next_bit(array->bitmap, MAX_RT_PRIO, idx+1) <
	       MAX_RT_PRIO)) {
		struct sched_rt_entity *rt_se;
		list_for_each_entry(rt_se, &array->queue[idx], run_list) {
			h_nr_running += check_rt_se(rt_se, rt_rq);
			nr_running++;
		}
	}

	BUG_ON(rt_rq->rt_nr_running != nr_running);
	return h_nr_running;
}

static void check_rq(struct rq *rq)
{
	unsigned long nr_running = 0;
	/* by sched_class, in order */
	if (rq->stop->on_rq) /* stop_sched_class */
		nr_running++;
	nr_running += check_rt_rq(&rq->rt); /* rt_sched_class */
	nr_running += check_cfs_rq(&rq->cfs); /* fair_sched_class */

	/* idle doesn't contribute to nr_running, so it isn't here */
	BUG_ON(nr_running != rq->nr_running);
}

void run_sanity_check(void)
{
	/*
	 * only bother checking one cpu at a time, any other cpus
	 * affected by the current tick will get checked whenever they
	 * run, which will likely be soon enough
	 */
	check_rq(cpu_rq(debug_smp_processor_id()));
}
