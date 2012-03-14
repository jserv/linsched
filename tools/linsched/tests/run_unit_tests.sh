#!/bin/bash
#
# Copyright 2011 Google Inc. All Rights Reserved.
# Author: asr@google.com (Abhishek Srivastava)

function die()
{
  echo $1 exit 1
}

seed_list=(100 1000 5000 10000)
n_list=(1000 10000 100000 1000000)

mu_gaussian_list=(10 20 100 200 500 1000)
sigma_gaussian_list=(5 10 100 200 500)

mu_poisson_list=(10 20 100 200 500 1000)

mu_exp_list=(10 20 100 200 500 1000)

mulog_lnorm_list=(2 3 4 5 6 7 8 9 10)
sdlog_lnorm_list=(1 2 3 4 5)

for i in "$@"; do
  if [ "$i" = "rand" ]; then
     for seed in ${seed_list[@]}; do
        for n in ${n_list[@]}; do
          echo -e "\nRunning rand with seed=$seed and n=$n"
             ./linsched_rand_test $i $seed $n
        done
     done
   elif [ "$i" = "gaussian" ]; then
     for seed in ${seed_list[@]}; do
       for mu in ${mu_gaussian_list[@]}; do
         for sigma in ${sigma_gaussian_list[@]}; do
           for n in ${n_list[@]}; do
             echo -e "\nRunning gaussian with seed=$seed,mu=$mu,sigma=$sigma,n=$n"
             ./linsched_rand_test $i $seed $mu $sigma $n
           done
         done
       done
     done
   elif [ "$i" = "poisson" ]; then
      for seed in ${seed_list[@]}; do
       for mu in ${mu_poisson_list[@]}; do
           for n in ${n_list[@]}; do
             echo -e "\nRunning poisson with seed=$seed,mu=$mu,n=$n"
             ./linsched_rand_test $i $seed $mu $n
           done
       done
     done
  elif [ "$i" = "exp" ]; then
      for seed in ${seed_list[@]}; do
       for mu in ${mu_exp_list[@]}; do
           for n in ${n_list[@]}; do
             echo -e "\nRunning exponential with seed=$seed,mu=$mu,n=$n"
             ./linsched_rand_test $i $seed $mu $n
           done
       done
     done
 elif [ "$i" = "lnorm" ]; then
     for seed in ${seed_list[@]}; do
       for mulog in ${mulog_lnorm_list[@]}; do
         for sdlog in ${sdlog_lnorm_list[@]}; do
           for n in ${n_list[@]}; do
             echo -e "\nRunning lognormal with seed=$seed,mu=$mulog,sigma=$sdlog,n=$n"
             ./linsched_rand_test $i $seed $mulog $sdlog $n
           done
         done
       done
     done
 else
    ./$i || die "$i failed"
  fi
done
