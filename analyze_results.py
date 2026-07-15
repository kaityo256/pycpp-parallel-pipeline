import os
import sys
import yaml
import numpy as np

def analyze_results(config):
    L = int(config["L"])
    start = float(config["start"])
    end = float(config["end"])
    num_points = int(config["num_points"])
    num_seeds = int(config["num_seeds"])
    output_dir = str(config["output_dir"])


    data = []
    for index in range(num_points):
        bond_probability = start + (end - start) * index / (num_points - 1)
        results = []
        for seed in range(num_seeds):
            probability_id = f"{round(bond_probability * 10000):04d}"
            input_filename = f"{output_dir}/L{L:03d}_p{probability_id}_s{seed:03d}.dat"
            print(input_filename)
            with open(input_filename, "r") as file:
                line = file.readline().strip()
                results.append(float(line))
            average = np.mean(results)
            std_error = np.std(results, ddof=1) / np.sqrt(len(results))
        data.append([
            bond_probability,
            average,
            std_error
        ])
    
    output_filename = os.path.join(output_dir,f"L{L:03d}.dat")
    np.savetxt(
    output_filename,
    data,
    fmt="%.10f",
    header=(
        "bond_probability "
        "percolation_probability "
        "std_error"
    )
)
    print("Saved analysis result to {}".format(output_filename))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 analyze_results.py input.yaml")
        sys.exit(1)

    config_file = sys.argv[1]

    try:
        with open(config_file, "r") as file:
            config = yaml.safe_load(file)

        analyze_results(config)

    except (OSError, ValueError, KeyError) as error:
        print("Error: {}".format(error), file=sys.stderr)
        sys.exit(1)

