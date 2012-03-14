/* Test library for the Linux Scheduler Simulator
 *
 * Basic assertions about tasks, ability to expect test failures.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see COPYING); if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef TEST_LIB_H

#include "linsched.h"

/* Creates a sleep/run task on the given cpus. Returns the new task. */
struct task_struct *create_task(unsigned long mask, int sleep, int run);
/* Creates count sleep/run tasks on the given cpus. */
void create_tasks(unsigned int count, unsigned long mask, int sleep, int run);

/* Require that the given condition be true. Equivalent to assert()
 * except that this plays nicely with expect_failure() */
#define expect(cond)  __expect(__FILE__, __LINE__, __PRETTY_FUNCTION__, #cond, (cond));

/* Expect that the next call to expect() or one of the wrapping
 * functions will fail (presumably due to a not-yet-fixed bug).
 *
 * Expected failures will not stop execution and will be reported at
 * exit. Unexpected successes are treated like a normal failure.
 */
#define expect_failure() (__expect_failure = 1)

/* Expect that the task given by pid has runtime within runtime +-
 * runtime_d, and the equivalent for wait and pcount. */
void expect_task_all(int pid, u64 runtime, u64 runtime_d,
		     u64 wait, u64 wait_d, unsigned int pcount,
		     unsigned int pcount_d);

/* expect_task_all for all tasks from lowpid up to and including
 * highpid. Using this with expect_failure means that at least one
 * task should fail */
void expect_tasks_all(int lowpid, int highpid, u64 runtime, u64 runtime_d,
		      u64 wait, u64 wait_d, unsigned int pcount,
		      unsigned int pcount_d);

#define MAX_RESULTS 8
void validate_results(int *expected_results);
/* Set information about the currently running test. Default test name is argv[0] */
void set_test_duration(long ticks);
void set_test_name(const char *name);

/* Implemented by user. Using this instead of test_main ensures that
 * warnings are printed in the case of expected failures */
void test_main(int argc, char **argv);

extern struct linsched_topology linsched_topo_db[MAX_TOPOLOGIES];

int parse_topology(char *arg);


extern int __expect_failure;
void __expect(const char *file, int line, const char *function,
	      const char *cond, int succeeded);

#endif
