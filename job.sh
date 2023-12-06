#!/bin/bash
# (See https://arc-ts.umich.edu/greatlakes/user-guide/ for command details)
# Set up batch job settings
#SBATCH --job-name=delta-ddmin
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=16
#SBATCH --exclusive
#SBATCH --time=00:02:00
#SBATCH --account=eecs587f23_class
#SBATCH --partition=standard
# { export OMP_NUM_THREADS=32; time ./delta > delta.stdout ; } 2> delta_time.txt
{ export OMP_NUM_THREADS=16; ./delta > delta.stdout ; } 2> delta.stderr