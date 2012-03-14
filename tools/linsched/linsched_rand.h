/* Copyright 2011 Google Inc. All Rights Reserved.
 Author: asr@google.com (Abhishek Srivastava) */

#ifndef __LINSCHED_RAND_H
#define __LINSCHED_RAND_H

#define MASK 123456789

enum RND_TYPE {
	GAUSSIAN,
	POISSON,
	EXPONENTIAL,
	LOGNORMAL,
	MAX_RND_TYPE = LOGNORMAL,
};

struct rand_dist {
	enum RND_TYPE type;
	void *dist;
	double (*gen_fn) (struct rand_dist * pdist);
};

struct gaussian_dist {
	int mu, sigma;
	unsigned int *rand_state;
};

struct poisson_dist {
	int mu;
	unsigned int *rand_state;
};

struct exp_dist {
	int mu;
	unsigned int *rand_state;
};

struct lognormal_dist {
	double meanlog, sdlog;
	struct rand_dist *std_gauss_dist;
};

int *linsched_init_rand(const unsigned int seed);
void linsched_destroy_rand(unsigned int *state);
double linsched_rand(unsigned int *const state);
double linsched_rand_range(double low, double high,
				unsigned int *const rand_state);
double linsched_gen_gaussian_dist(struct rand_dist *rdist);
double gammaln(const double xx);
double linsched_gen_poisson_dist(struct rand_dist *rdist);
double linsched_gen_exp_dist(struct rand_dist *rdist);
double linsched_gen_lognormal_dist(struct rand_dist *rdist);
struct rand_dist *linsched_copy_dist(const struct rand_dist *rdist,
				     unsigned int *rand_state);
struct rand_dist *linsched_init_gaussian(const int mu, const int sigma,
					 const int seed);
void linsched_destroy_gaussian(struct rand_dist *rdist);
struct rand_dist *linsched_init_poisson(const int mu, const int seed);
void linsched_destroy_poisson(struct rand_dist *rdist);
struct rand_dist *linsched_init_exponential(const double mu, const int seed);
void linsched_destroy_exponential(struct rand_dist *rdist);
struct rand_dist *linsched_init_lognormal(const double meanlog,
					const double sdlog, const int seed);
void linsched_destroy_lognormal(struct rand_dist *rdist);
void linsched_destroy_dist(struct rand_dist *rdist);

#endif				/* __LINSCHED_RAND_H */
