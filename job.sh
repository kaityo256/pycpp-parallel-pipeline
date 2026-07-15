#!/bin/sh
#SBATCH -p i8cpu
#SBATCH -N 1
#SBATCH -n 128

srun ./cps/cps task.sh
