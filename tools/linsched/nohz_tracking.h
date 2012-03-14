#ifndef NOHZ_TRACKING_H
#define NOHZ_TRACKING_H

/* called for each cpu, for each tick */
void track_nohz_residency(int cpu);
void print_nohz_residency(void);

#endif
