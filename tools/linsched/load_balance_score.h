#ifndef LOAD_BALANCE_SCORE_H
#define LOAD_BALANCE_SCORE_H

#include <stdio.h>

void init_lb_info(void);
void compute_lb_info(void);
double get_average_imbalance(void);
void dump_lb_info(FILE *out);

#endif
