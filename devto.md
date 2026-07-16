---
title: "A Python/C++ Pipeline for Embarrassingly Parallel Simulations"
published: false
description: "Use Python to generate many independent simulation jobs, C++ to run the compute-heavy kernel, and Python again to analyze the results."
tags: python, cpp, hpc, simulation
---

Numerical simulations often have a simple but important structure: run the same program many times with different parameters, then combine the results.

For example, when scanning a phase diagram, each parameter point can usually be calculated independently. If there are 100 parameter values, there are 100 independent jobs. If each parameter value also needs multiple random seeds, the number of independent jobs grows even further.

This is an embarrassingly parallel workload. The parallelism is conceptually easy, but the surrounding workflow can become messy:

- How should input files be generated?
- How should each job be named?
- How should the compute-heavy part be implemented?
- How should hundreds or thousands of output files be collected?
- How can the workflow be rerun later without forgetting the exact parameters?

This repository demonstrates one compact answer:

https://github.com/kaityo256/pycpp-parallel-pipeline

It uses a two-dimensional bond percolation calculation as the example problem, but the main point is not percolation itself. The main point is the reusable pipeline:

1. Write the workflow parameters in one YAML file.
2. Use Python to generate many small input files.
3. Run one C++ simulation program per input file.
4. Distribute those commands in parallel with `cps`.
5. Use Python to aggregate the output files.
6. Plot the summarized result with gnuplot.

## Why Split the Workflow This Way?

Python and C++ are useful for different parts of the job.

Python is convenient for orchestration:

- reading YAML
- creating directories
- generating many input files
- collecting result files
- calculating averages and standard errors

C++ is useful for the compute-heavy kernel:

- tight loops
- random sampling
- memory-efficient data structures
- predictable compiled performance

The workflow keeps these responsibilities separate. Python prepares and analyzes. C++ computes. The parallel scheduler only needs a plain task file containing one command per line.

That separation makes the repository easy to adapt to other simulation problems.

## Repository Structure

The important files are:

```text
generate_inputs.py       # Generate per-job YAML files and task.sh
analyze_results.py       # Read .dat files and compute statistics
input.yaml               # Top-level workflow configuration
cpp/main.cpp             # C++ entry point
cpp/percolation.hpp      # Bond percolation implementation
cpp/param                # C++ parameter-file parser submodule
cps                      # Parallel command scheduler submodule
plot.plt                 # gnuplot script
fig/L128.png             # Example result plot
```

The `cps` and `cpp/param` directories are Git submodules, so clone the repository recursively:

```console
git clone --recursive https://github.com/kaityo256/pycpp-parallel-pipeline.git
cd pycpp-parallel-pipeline
```

If the repository was already cloned without submodules:

```console
git submodule update --init --recursive
```

## The Top-Level Configuration

The whole run starts from `input.yaml`:

```yaml
L: 128
start: 0.45
end: 0.55
num_points: 20
num_samples: 100
num_seeds: 10
output_dir: output
```

Here:

- `L` is the linear system size.
- `start` and `end` define the range of bond-open probabilities.
- `num_points` is the number of probability values to scan.
- `num_samples` is the number of Monte Carlo samples in one C++ job.
- `num_seeds` is the number of independent random seeds per probability value.
- `output_dir` is where generated inputs and results are stored.

With this configuration, Python generates:

```text
num_points * num_seeds = 20 * 10 = 200
```

independent simulation jobs.

## Generating Input Files with Python

The script `generate_inputs.py` reads `input.yaml`, computes the list of bond probabilities, and writes one YAML input file per job.

The generated files look like this:

```text
output/L128_p4500_s000.yaml
output/L128_p4500_s001.yaml
...
output/L128_p5500_s009.yaml
```

The filename encodes the important parameters:

- `L128` means `L = 128`.
- `p4500` means `bond_probability = 0.4500`.
- `s000` means `seed = 0`.

Each generated YAML file contains the parameters needed by one C++ process:

```yaml
L: 128
bond_probability: 0.45
seed: 0
num_samples: 100
output_dir: output
```

The script also writes `task.sh`, a plain command list:

```sh
./cpp/percolation output/L128_p4500_s000.yaml
./cpp/percolation output/L128_p4500_s001.yaml
./cpp/percolation output/L128_p4500_s002.yaml
```

This design is intentionally simple. A task file like this can be inspected, debugged, run sequentially, or passed to a scheduler.

Generate the inputs with:

```console
python3 generate_inputs.py input.yaml
```

Or, when using `uv`:

```console
uv run python generate_inputs.py input.yaml
```

## The C++ Simulation Kernel

The C++ side receives exactly one input file:

```console
cpp/percolation output/L128_p4500_s000.yaml
```

The entry point in `cpp/main.cpp` is deliberately small. It checks the command-line argument, reads the parameter file with `param`, and calls the simulation task:

```cpp
const std::string filename = argv[1];
param::parameter param(filename);
percolation::run_task(param);
```

The actual model lives in `cpp/percolation.hpp`.

For each sample, the code creates a square `L x L` lattice and opens nearest-neighbor bonds with probability `bond_probability`. It uses a union-find style cluster representation:

- `find` returns the root of a cluster.
- `unite` merges two connected sites.
- `check_clustering` checks whether the bottom row and top row are connected.

The percolation probability is estimated by repeated sampling:

```cpp
double estimate_percolation_probability(
    int L,
    double bond_probability,
    int seed,
    int num_samples) {

  std::mt19937_64 rng(seed);
  int num_percolated = 0;

  for (int sample = 0; sample < num_samples; ++sample) {
    if (does_percolate(L, bond_probability, rng)) {
      ++num_percolated;
    }
  }

  return static_cast<double>(num_percolated) / num_samples;
}
```

Each C++ process writes one `.dat` file containing one number: the estimated percolation probability for that parameter and seed.

For example:

```text
output/L128_p4500_s000.dat
```

## Building the Tools

The repository contains two compiled components:

- `cps`, the parallel command scheduler
- `cpp/percolation`, the simulation program

Build `cps`:

```console
cd cps
make
cd ..
```

Build the C++ simulation:

```console
cd cpp
make
cd ..
```

After that, these executables should exist:

```text
cps/cps
cpp/percolation
```

## Parallel Execution with cps

The scheduler `cps` reads a task file and distributes commands among MPI processes.

For example:

```console
mpirun -np 5 cps/cps task.sh
```

One MPI process is used by `cps` for scheduling, so `-np 5` gives four worker processes.

On a cluster using Slurm, the same idea can be wrapped in a job script:

```sh
#!/bin/sh
#SBATCH -p i8cpu
#SBATCH -N 1
#SBATCH -n 128

srun ./cps/cps task.sh
```

The scheduler assigns commands to available workers. When a worker finishes one simulation, it receives the next command from the task file.

This is useful when individual tasks do not all take exactly the same amount of time. A large pool of small tasks naturally improves load balancing.

## Analyzing Results with Python

After all C++ jobs finish, the output directory contains one `.dat` file per input file.

The analysis script is run with the same top-level configuration:

```console
python3 analyze_results.py input.yaml
```

This is an important detail. The script does not need to guess which files should exist by blindly scanning the directory. Instead, it reconstructs the expected filenames from the same parameters that created the jobs.

For each bond probability, it reads the results from all seeds and computes:

- the mean percolation probability
- the standard error of the mean

The summarized output is written to a file such as:

```text
output/L128.dat
```

Its format is:

```text
# bond_probability percolation_probability std_error
0.4500000000 0.0000000000 0.0000000000
0.4552631579 0.0000000000 0.0000000000
0.4605263158 0.0000000000 0.0000000000
...
0.5500000000 1.0000000000 0.0000000000
```

## Plotting

The repository includes a gnuplot script:

```console
gnuplot plot.plt
```

It reads the summarized data and writes:

```text
fig/L128.png
```

The resulting curve shows the transition of the percolation probability around the expected threshold for two-dimensional bond percolation.

## Complete Workflow

Using `uv`, the whole workflow is:

```console
git submodule update --init --recursive

uv venv
source .venv/bin/activate
uv pip install numpy pyyaml

cd cps
make
cd ..

cd cpp
make
cd ..

python3 generate_inputs.py input.yaml
mpirun -np 5 cps/cps task.sh
python3 analyze_results.py input.yaml
gnuplot plot.plt
```

Using the standard library `venv` instead:

```console
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install --upgrade pip
python3 -m pip install numpy pyyaml
```

The remaining commands are the same.

## Practical Lessons

The example is small, but the pattern scales well.

Keep workflow parameters in a file, not in command-line arguments. If a result needs to be reproduced later, a saved YAML file is much easier to audit than a shell history entry.

Generate explicit input files. They are useful records of what was actually submitted.

Use many short independent jobs when possible. For Monte Carlo simulations, ten jobs with different seeds can be more convenient than one long job with ten times as many samples. Shorter jobs fit scheduler limits more easily, and a large task pool improves load balancing.

Make output filenames deterministic. If the filename encodes the key parameters, debugging and post-processing become much easier.

Analyze from the configuration, not from a loose directory scan. A directory may contain stale files from previous runs. Reconstructing expected filenames from the original configuration reduces that risk.

## Adapting the Pattern

To reuse this pipeline for another simulation, replace only three parts:

1. `generate_inputs.py`: write the per-task parameters your simulation needs.
2. `cpp/percolation.hpp`: replace the percolation model with your compute kernel.
3. `analyze_results.py`: aggregate your simulation outputs into the statistics you need.

The surrounding structure can stay almost the same:

- one top-level YAML file
- many per-task input files
- one executable command per task
- one result file per task
- one analysis script

That is the core idea: keep the computational kernel focused, and make the workflow around it explicit and reproducible.

## Suggested Keywords

- Python C++ workflow
- embarrassingly parallel
- high performance computing
- numerical simulation
- Monte Carlo simulation
- MPI
- Slurm
- YAML configuration
- reproducible research
- percolation
