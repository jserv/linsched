/* Copyright 2011 Google Inc. All Rights Reserved.
 * Author: asr@google.com (Abhishek Srivastava)
 */

#include "linsched_rand.h"
#include <math.h>
#include <stdlib.h>

/* seed the linsched_rand()
 * XORing with MASK to allow seed to be 0 */
int *linsched_init_rand(const unsigned int seed)
{
	unsigned int *state = malloc(sizeof(unsigned int));
	*state = seed ^ MASK;
	return state;
}

/* destroy() required after init() */
void linsched_destroy_rand(unsigned int *state)
{
	if (state)
		free(state);
}

/* * Lehmer random number generator (ref. Knuth (AoCP) )
 * * call linsched_srand() before calling linsched_rand()
 * * n = 2147483647 (Mersenne Prime)
 * * g = 16807 */
double linsched_rand(unsigned int *const state)
{
	unsigned int rand = (16807 * (*state)) % 2147483647;
	*state = rand;
	return rand / (double) 2147483647;
}

/* return a random number from [low, high)
 * low and high must be positive
 * rand_state must be initialized by linsched_init_rand()  */
double linsched_rand_range(double low, double high,
			unsigned int *rand_state)
{
	double rval = linsched_rand(rand_state);
	return low + rval * (high - low);
}

/* generates a gaussian deviate with given parameters
 * (ref GSL's gsl_ran_gaussian) */
double linsched_gen_gaussian_dist(struct rand_dist *rdist)
{

	int mu = ((struct gaussian_dist *) rdist->dist)->mu;
	int sigma = ((struct gaussian_dist *) rdist->dist)->sigma;
	unsigned int *rand_state =
	    ((struct gaussian_dist *) rdist->dist)->rand_state;

	double x, y, r2;

	do {
		x = -1 + 2 * linsched_rand(rand_state);
		y = -1 + 2 * linsched_rand(rand_state);

		r2 = x * x + y * y;
	} while (r2 > 1.0 || r2 == 0);

	return mu + sigma * y * sqrt(-2.0 * log(r2) / r2);
}

/* takes the log of gamma(xx)
 * (ref. Numerical Recipies in C : Pg 214 */
double gammaln(const double xx)
{
	double x, y, tmp, ser;
	static double cof[6] =
	    { 76.18009172947146, -86.50532032941677, 24.01409824083091,
		-1.231739572450155, 0.1208650973866170e-2,
		-0.5395239384953e-5
	};
	int j;

	y = x = xx;
	tmp = x + 5.5;
	tmp -= (x + 0.5) * log(tmp);
	ser = 1.000000000190015;

	for (j = 0; j <= 5; j++)
		ser += cof[j] / ++y;
	return -tmp + log(2.5066282746310005 * ser / x);

}

/* generates a poisson distribution given mean mu
 * (ref. Numerical Recipies in C : Pg 294 */
double linsched_gen_poisson_dist(struct rand_dist *rdist)
{
	int mu = ((struct poisson_dist *) rdist->dist)->mu;
	unsigned int *rand_state =
	    ((struct poisson_dist *) rdist->dist)->rand_state;

	double sq = 0.0, alxm = 0.0, g = 0.0, oldm = (-1.0);
	double em, t, y;

	if (mu < 12.0) {
		if (mu != oldm) {
			oldm = mu;
			g = exp(-mu);
		}
		em = -1;
		t = 1.0;
		do {
			++em;
			t *= linsched_rand(rand_state);
		} while (t > g);

	} else {
		if (mu != oldm) {
			oldm = mu;
			sq = sqrt(2.0 * mu);
			alxm = log(mu);
			g = mu * alxm - gammaln(mu + 1.0);
		}
		do {
			do {
				y = tan(M_PI * linsched_rand(rand_state));
				em = sq * y + mu;
			} while (em < 0.0);
			em = floor(em);
			t = 0.9 * (1.0 + y * y) * exp(em * alxm -
						      gammaln(em + 1.0)
						      - g);
		} while (linsched_rand(rand_state) > t);
	}
	return em;
}

/* generates an exponential distribution given mean mu
 * (ref. Numerical Recipies in C : Pg 287 */
double linsched_gen_exp_dist(struct rand_dist *rdist)
{
	double edev;
	double mu = ((struct exp_dist *) rdist->dist)->mu;

	unsigned int *rand_state =
	    ((struct exp_dist *) rdist->dist)->rand_state;

	do {
		edev = linsched_rand(rand_state);
	} while (edev == 0.0);

	return -mu * log(edev);
}

/* generates lognormal deviates using a std gaussian N(0,1)
 * ref. wikipedia (http://en.wikipedia.org/wiki/Log-normal_distribution) */
double linsched_gen_lognormal_dist(struct rand_dist *rdist)
{
	double lndev, gaussdev;
	struct lognormal_dist *ldist = ((struct lognormal_dist *) rdist->dist);
	double meanlog = ldist->meanlog;
	double sdlog = ldist->sdlog;
	struct rand_dist *gdist = ((struct rand_dist *) ldist->std_gauss_dist);

	gaussdev = linsched_gen_gaussian_dist(gdist);
	lndev = exp(meanlog + gaussdev * sdlog);
	return lndev;
}

struct rand_dist *linsched_init_gaussian(const int mu, const int sigma,
					 const int seed)
{
	struct rand_dist *rdist = malloc(sizeof(struct rand_dist));
	struct gaussian_dist *gdist = malloc(sizeof(struct gaussian_dist));

	gdist->mu = mu;
	gdist->sigma = sigma;
	gdist->rand_state = linsched_init_rand(seed);
	rdist->type = GAUSSIAN;
	rdist->gen_fn = linsched_gen_gaussian_dist;
	rdist->dist = gdist;
	return rdist;
}

void linsched_destroy_gaussian(struct rand_dist *rdist)
{
	unsigned int *rand_state =
	    ((struct gaussian_dist *) rdist->dist)->rand_state;

	if (rdist) {
		if (rdist->dist) {
			linsched_destroy_rand(rand_state);
			free(rdist->dist);
		}
		free(rdist);
	}
}

struct rand_dist *linsched_init_poisson(const int mu, const int seed)
{
	struct rand_dist *rdist = malloc(sizeof(struct rand_dist));
	struct poisson_dist *pdist = malloc(sizeof(struct poisson_dist));

	pdist->mu = mu;
	pdist->rand_state = linsched_init_rand(seed);
	rdist->type = POISSON;
	rdist->gen_fn = linsched_gen_poisson_dist;
	rdist->dist = pdist;
	return rdist;
}

void linsched_destroy_poisson(struct rand_dist *rdist)
{

	unsigned int *rand_state =
	    ((struct poisson_dist *) rdist->dist)->rand_state;

	if (rdist) {
		if (rdist->dist) {
			linsched_destroy_rand(rand_state);
			free(rdist->dist);
		}
		free(rdist);
	}
}

struct rand_dist *linsched_init_exponential(const double mu, const int seed)
{
	struct rand_dist *rdist = malloc(sizeof(struct rand_dist));
	struct exp_dist *edist = malloc(sizeof(struct exp_dist));

	edist->mu = mu;
	edist->rand_state = linsched_init_rand(seed);
	rdist->type = EXPONENTIAL;
	rdist->gen_fn = linsched_gen_exp_dist;
	rdist->dist = edist;
	return rdist;
}

void linsched_destroy_exponential(struct rand_dist *rdist)
{
	unsigned int *rand_state;

	if (rdist) {
		if (rdist->dist) {
			rand_state =
			    ((struct exp_dist *) rdist->dist)->rand_state;
			linsched_destroy_rand(rand_state);
			free(rdist->dist);
		}
		free(rdist);
	}
}

struct rand_dist *linsched_init_lognormal(const double meanlog,
					  const double sdlog, const int seed)
{
	struct rand_dist *rdist = malloc(sizeof(struct rand_dist));
	struct lognormal_dist *ldist = malloc(sizeof(struct lognormal_dist));

	ldist->meanlog = meanlog;
	ldist->sdlog = sdlog;
	ldist->std_gauss_dist = linsched_init_gaussian(0, 1, seed);
	rdist->type = LOGNORMAL;
	rdist->gen_fn = linsched_gen_lognormal_dist;
	rdist->dist = ldist;
	return rdist;
}

void linsched_destroy_lognormal(struct rand_dist *rdist)
{
	struct lognormal_dist *ldist = ((struct lognormal_dist *) rdist->dist);
	struct rand_dist *gdist = ((struct rand_dist *) ldist->std_gauss_dist);

	if (rdist) {
		if (rdist->dist) {
			linsched_destroy_gaussian(gdist);
			free(ldist);
		}
		free(rdist);
	}

}

void linsched_destroy_dist(struct rand_dist *rdist)
{
       switch (rdist->type) {
       case GAUSSIAN:
               linsched_destroy_gaussian(rdist);
               break;
       case POISSON:
               linsched_destroy_poisson(rdist);
               break;
       case EXPONENTIAL:
               linsched_destroy_exponential(rdist);
               break;
       case LOGNORMAL:
               linsched_destroy_lognormal(rdist);
               break;
       }
}

struct rand_dist *linsched_copy_dist(const struct rand_dist *rdist,
				     unsigned int *rand_state)
{
	linsched_rand(rand_state);
	switch (rdist->type) {
	case GAUSSIAN: {
		struct gaussian_dist *dist = rdist->dist;
		return linsched_init_gaussian(dist->mu, dist->sigma,
						 *rand_state);
	}
	case POISSON: {
		struct poisson_dist *dist = rdist->dist;
		return linsched_init_poisson(dist->mu, *rand_state);
	}
	case EXPONENTIAL: {
		struct exp_dist *dist = rdist->dist;
		return linsched_init_exponential(dist->mu, *rand_state);
	}
	case LOGNORMAL: {
		struct lognormal_dist *dist = rdist->dist;
		return linsched_init_lognormal(dist->meanlog, dist->sdlog,
					       *rand_state);
	}
	default:
		return NULL;
	}
}
