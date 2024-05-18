import matplotlib.pyplot as plt
import numpy as np

# Data for the chart
num_cities = [4, 5, 6, 7, 8, 9, 10]

# Global Search results
global_costs = [302, 261, 319, 365, 454, 553, 422]
global_times = [0, 0, 3, 20, 348, 3960, 37638]

# Parallel Global Search results
parallel_global_costs = [302, 261, 319, 365, 454, 553, 422]
parallel_global_times = [16, 8, 2, 11, 151, 1546, 18103]

# Local Search results
local_costs = [326, 281, 369, 430, 540, 571, 526]
local_times = [444, 525, 644, 741, 899, 1033, 1288]

# Parallel Local Search results
parallel_local_costs = [326, 281, 369, 430, 574, 571, 526]
parallel_local_times = [56, 40, 49, 60, 68, 77, 100]

# Parallel Local Search with MPI results
parallel_mpi_costs = [326, 281, 369, 430, 556, 571, 526]
parallel_mpi_times = [580, 160, 149, 156, 216, 156, 90]

# Plotting the costs
plt.figure(figsize=(21, 7))
plt.subplot(1, 3, 1)
plt.plot(num_cities, global_costs, label='Global Search')
plt.plot(num_cities, parallel_global_costs, label='Parallel Global Search')
plt.plot(num_cities, local_costs, label='Local Search')
plt.plot(num_cities, parallel_local_costs, label='Parallel Local Search')
plt.plot(num_cities, parallel_mpi_costs, label='Parallel Local Search with MPI')
plt.xlabel('Number of Cities')
plt.ylabel('Best Route Cost')
plt.title('CVRP Algorithm Best Route Costs')
plt.legend()
plt.grid(True)

# Plotting the times
plt.subplot(1, 3, 2)
plt.plot(num_cities, global_times, label='Global Search')
plt.plot(num_cities, parallel_global_times, label='Parallel Global Search')
plt.plot(num_cities, local_times, label='Local Search')
plt.plot(num_cities, parallel_local_times, label='Parallel Local Search')
plt.plot(num_cities, parallel_mpi_times, label='Parallel Local Search with MPI')
plt.xlabel('Number of Cities')
plt.ylabel('Time Taken (ms)')
plt.title('CVRP Algorithm Times')
plt.legend()
plt.grid(True)

# Plotting the times on a logarithmic scale
plt.subplot(1, 3, 3)
plt.plot(num_cities, global_times, label='Global Search')
plt.plot(num_cities, parallel_global_times, label='Parallel Global Search')
plt.plot(num_cities, local_times, label='Local Search')
plt.plot(num_cities, parallel_local_times, label='Parallel Local Search')
plt.plot(num_cities, parallel_mpi_times, label='Parallel Local Search with MPI')
plt.xlabel('Number of Cities')
plt.ylabel('Time Taken (ms) (log scale)')
plt.yscale('log')
plt.title('CVRP Algorithm Times (Log Scale)')
plt.legend()
plt.grid(True, which="both", ls="--")

plt.tight_layout()
plt.show()
