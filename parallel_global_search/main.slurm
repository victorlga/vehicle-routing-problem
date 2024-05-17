#!/bin/bash
#SBATCH --job-name=cap_vehicle_routing
#SBATCH --output=result_%j.txt        # Output file name, where %j is the job ID
#SBATCH --partition=espec            # Explicitly specify the partition if needed
#SBATCH --nodes=2                     # Total number of nodes requested
#SBATCH --ntasks-per-node=16          # Number of tasks per node
#SBATCH --cpus-per-task=1             # Number of CPUs per task
#SBATCH --mem-per-cpu=200M            # Memory per CPU core

# Calculate total number of MPI processes to start
total_procs=$((SLURM_NTASKS_PER_NODE * SLURM_NNODES))

# Run the program using mpirun or mpiexec; adjust depending on your MPI implementation
mpirun -np $total_procs ./CVRP_Solver
