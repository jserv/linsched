/* Copyright 2011 Google Inc. All Rights Reserved.
 * Author: asr@google.com (Abhishek Srivastava)
 *
 * Tests for the LinSched rand(), gaussian and poisson generators
 **/
#include <linux/kernel.h>

#include <string.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "linsched_rand.h"

double calc_deviation(double avg, double sum_sq, int n)
{
	return sqrt((sum_sq / n) - (avg * avg));
}

/* test the linsched_rand() function
 * Note : a fair random number generator should have mu close to 0.5
 * and sd = 0.25
 * */
int test_linsched_rand(int argc, char **argv)
{
	unsigned int seed =
	    (unsigned int) simple_strtoul(argv[2], NULL, 0);
	int n = simple_strtoul(argv[3], NULL, 0);
	int *state = linsched_init_rand(seed);
	int i;
	double sum = 0, sum_sq = 0;
	double val, avg;

	for (i = 0; i < n; i++) {
		val = linsched_rand(state);
		sum += val;
		sum_sq += (val * val);
		fprintf(stdout, "val = %f\n", val);
	}

	avg = sum / (double) n;

	fprintf(stdout, "\n AVG(mu) : %f, DEVIATION(sd) = %f\n", avg,
		calc_deviation(avg, sum_sq, n));
	linsched_destroy_rand(state);
	return 0;
}

/* test the linsched_rand_range() function */
int test_linsched_rand_range(int argc, char **argv)
{
	unsigned int seed =
	    (unsigned int) simple_strtoul(argv[2], NULL, 0);
	unsigned long low = simple_strtoul(argv[3], NULL, 0);
	unsigned long high = simple_strtoul(argv[4], NULL, 0);
	int n = simple_strtoul(argv[5], NULL, 0);
	unsigned int *state = linsched_init_rand(seed);
	int i;
	double sum = 0, sum_sq = 0, avg, val;

	for (i = 0; i < n; i++) {
		val = linsched_rand_range(low, high, state);
		sum += val;
		sum_sq += (val * val);
		fprintf(stdout, "val = %f\n", val);
	}

	avg = sum / (double) n;

	fprintf(stdout, "AVG(mu) : %f, DEVIATION(sd) = %f\n", avg,
		calc_deviation(avg, sum_sq, n));
	linsched_destroy_rand(state);
	return 0;

}

/* test the linsched_create_gaussian_dist() function */
int test_linsched_gaussian(int argc, char **argv)
{
	unsigned int seed =
	    (unsigned int) simple_strtoul(argv[2], NULL, 0);
	int mu = simple_strtoul(argv[3], NULL, 0);
	int sd = simple_strtoul(argv[4], NULL, 0);
	int n = simple_strtoul(argv[5], NULL, 0);
	int i, val;
	struct rand_dist *rdist = linsched_init_gaussian(mu, sd, seed);

	long sum = 0, sum_sq = 0;
	double avg;

	for (i = 0; i < n; i++) {
		val = linsched_gen_gaussian_dist(rdist);
		sum += val;
		sum_sq += (val * val);
	}

	avg = sum / (double) n;

	fprintf(stdout, "\n AVG(mu) : %f, DEVIATION(sd) = %f\n", avg,
		calc_deviation(avg, sum_sq, n));
	linsched_destroy_gaussian(rdist);
	return 0;
}

/* test the linsched_create_poisson_dist() function */
int test_linsched_poisson(int argc, char **argv)
{
	unsigned int seed =
	    (unsigned int) simple_strtoul(argv[2], NULL, 0);
	int mu = simple_strtoul(argv[3], NULL, 0);
	int n = simple_strtoul(argv[4], NULL, 0);
	int i, val;
	struct rand_dist *rdist = linsched_init_poisson(mu, seed);

	long sum = 0, sum_sq = 0;
	double avg;

	for (i = 0; i < n; i++) {
		val = linsched_gen_poisson_dist(rdist);
		sum += val;
		sum_sq += (val * val);
	}

	avg = sum / (double) n;

	fprintf(stdout, "\n AVG(mu) : %f, DEVIATION(sd) = %f\n", avg,
		calc_deviation(avg, sum_sq, n));
	linsched_destroy_poisson(rdist);
	return 0;

}

/* test the linsched_gen_exp_dist() function */
int test_linsched_exponential(int argc, char **argv)
{
	unsigned int seed =
	    (unsigned int) simple_strtoul(argv[2], NULL, 0);
	int mu = simple_strtoul(argv[3], NULL, 0);
	int n = simple_strtoul(argv[4], NULL, 0);
	struct rand_dist *rdist = linsched_init_exponential(mu, seed);
	long sum = 0, sum_sq = 0;
	int i;
	double avg, val;

	for (i = 0; i < n; i++) {
		val = linsched_gen_exp_dist(rdist);
		sum += val;
		sum_sq += (val * val);
	}

	avg = sum / (double) n;
	fprintf(stdout, "\n AVG(mu) : %f, DEVIATION(sd) = %f\n", avg,
		calc_deviation(avg, sum_sq, n));
	linsched_destroy_exponential(rdist);
	return 0;

}

int test_linsched_lognormal(int argc, char **argv)
{
	unsigned int seed =
	    (unsigned int) simple_strtoul(argv[2], NULL, 0);
	int meanlog = simple_strtoul(argv[3], NULL, 0);
	int sdlog = simple_strtoul(argv[4], NULL, 0);
	int n = simple_strtoul(argv[5], NULL, 0);
	int i;
	double sdlog_sq = pow(sdlog, 2);
	double exp_mean = exp(meanlog + 0.5 * sdlog_sq);
	double exp_sd = sqrt((exp(sdlog_sq) - 1) *
			(exp(2 * meanlog + sdlog_sq)));
	struct rand_dist *rdist = linsched_init_lognormal(meanlog, sdlog, seed);
	double avg, val, sum = 0, sum_sq = 0;

	for (i = 0; i < n; i++) {
		val = linsched_gen_lognormal_dist(rdist);
		sum += val;
		sum_sq += (val * val);
	}

	avg = sum / (double) n;
	fprintf(stdout, "\nAVG(mu): %f, DEVIATION(sd) = %f\n", avg,
		calc_deviation(avg, sum_sq, n));
	fprintf(stdout, "E.AVG:   %f, Exp. DEV      = %f\n", exp_mean,
		exp_sd);
	linsched_destroy_lognormal(rdist);
	return 0;
}

void print_usage(char *cmd)
{
	printf("\nUsage: %s <TEST_TYPE> [[PARAMETERS]]\n", cmd);
	printf("\t TEST_TYPE : rand / rand_range / gaussian / poisson / exp /"
								"lnorm\n");
	printf("\t PARAMETERS for rand : seed NO_OF_SAMPLES\n");
	printf("\t PARAMETERS for rand_range : seed <LOW> <HIGH>"
						" NO_OF_SAMPLES\n");
	printf
	    ("\t PARAMETERS for gaussian : seed mu sigma NO_OF_SAMPLES\n");
	printf("\t PARAMETERS for poisson : seed mu NO_OF_SAMPLES\n");
	printf("\t PARAMETERS for exponential : seed mu NO_OF_SAMPLES\n");
	printf("\t PARAMETERS for lognormal :"
			" seed meanlog sdlog NO_OF_SAMPLES\n");
	printf("EXAMPLE : %s rand 12345 10000\n", cmd);
	printf("EXAMPLE : %s gaussian 12345 1000 10 10000\n", cmd);
	printf("EXAMPLE : %s poisson 12345 500 10000\n", cmd);
	printf("EXAMPLE : %s exp 12345 500 10000\n", cmd);
	printf("EXAMPLE : %s lnorm 12345 1000 10 10000\n", cmd);
}

/* testing linsched_rand()
 *      argv[1] : rand
 *      argv[2] : seed for rand()
 *      argv[3] : number of samples
 * testing linsched_create_gaussian_dist()
 *      argv[1] : gaussian
 *      argv[2] : seed for rand()
 *      argv[3] : mean for gaussian (mu)
 *      argv[4] : sd for gaussian (sd)
 *      argv[5] : number of samples
 * testing linsched_create_poisson_dist()
 *      argv[1] : poisson
 *      argv[2] : seed for rand()
 *      argv[3] : mean for poisson (mu)
 *      argv[4] : number of samples
 */
int linsched_test_main(int argc, char **argv)
{
	if (argc == 4 && !strcmp(argv[1], "rand"))
		test_linsched_rand(argc, argv);

	else if (argc == 6 && !strcmp(argv[1], "rand_range"))
		test_linsched_rand_range(argc, argv);

	else if (argc == 6 && !strcmp(argv[1], "gaussian"))
		test_linsched_gaussian(argc, argv);

	else if (argc == 5 && !strcmp(argv[1], "poisson"))
		test_linsched_poisson(argc, argv);

	else if (argc == 5 && !strcmp(argv[1], "exp"))
		test_linsched_exponential(argc, argv);

	else if (argc == 6 && !strcmp(argv[1], "lnorm"))
		test_linsched_lognormal(argc, argv);

	else
		print_usage(argv[0]);

	return 0;
}
