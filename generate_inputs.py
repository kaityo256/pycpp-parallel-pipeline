from pathlib import Path
import yaml
import sys

def generate_tasks(files):
    for filename in files:
        print(f"./cpp/percolation {filename}")

def generate_inputs(config):
    L = int(config["L"])
    start = float(config["start"])
    end = float(config["end"])
    num_points = int(config["num_points"])
    num_samples = int(config["num_samples"])
    num_seeds = int(config["num_seeds"])
    output_directory = Path("output/input")

    output_directory.mkdir(parents=True, exist_ok=True)
    files = []
    for index in range(num_points):
        bond_probability = start + (end - start) * index / (num_points - 1)
        probability_id = f"{round(bond_probability * 10000):04d}"

        for seed in range(num_seeds):
            filename = (
                output_directory
                / f"L{L}_p{probability_id}_s{seed:03d}.yaml"
            )

            parameters = {
                "L": L,
                "bond_probability": bond_probability,
                "seed": seed,
                "num_samples": num_samples,
            }

            with filename.open("w", encoding="utf-8") as file:
                yaml.safe_dump(parameters, file)
            files.append(filename)

    print(f"Generated {len(files)} input files in {output_directory}")
    generate_tasks(files)

if __name__ == "__main__":
    if(len(sys.argv)!=2):
        print("Usage: python3 generate_inputs.py input.yaml")
        exit(1)
    config_file = sys.argv[1]
    print(config_file)
    with open(config_file) as file:
        config = yaml.safe_load(file)
    generate_inputs(config)