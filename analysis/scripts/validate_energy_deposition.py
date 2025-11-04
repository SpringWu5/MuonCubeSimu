#!/usr/bin/env python3
import argparse
import json
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import os

def main():
    """
    Simulates a physics analysis process. It generates mock data, creates a plot
    and a log file, and then produces the results_manifest.json file which acts
    as a contract for the post-processing script.
    """
    parser = argparse.ArgumentParser(description="Validate energy deposition and generate a results manifest.")
    parser.add_argument("--input-file", required=True, help="Path to the input .root file (mocked for this script).")
    parser.add_argument("--output-dir", required=True, help="Directory to save the analysis results.")
    args = parser.parse_args()

    output_dir = Path(args.output_dir)
    input_file = Path(args.input_file)

    # --- 1. Create the output directory if it doesn't exist ---
    output_dir.mkdir(parents=True, exist_ok=True)
    print(f"Created output directory: {output_dir}")

    # --- 2. Mock Analysis: Simulate reading a ROOT file and analyzing data ---
    # In a real scenario, you would use uproot here to open args.input_file
    print(f"Simulating analysis for input file: {input_file.name}...")
    # Generate some normally distributed mock data representing energy deposition
    np.random.seed(42)
    mean_energy = 45.8
    std_dev = 5.2
    num_hits = 2500000
    mock_energy_data = np.random.normal(loc=mean_energy, scale=std_dev, size=num_hits)

    # --- 3. Generate Artifacts ---
    # a) Create a plot
    plot_path = output_dir / "energy_spectrum.png"
    plt.figure(figsize=(10, 6))
    plt.hist(mock_energy_data, bins=200, histtype='step', linewidth=2, label='Simulated Energy Deposition')
    plt.title("Energy Deposition Spectrum")
    plt.xlabel("Energy (MeV)")
    plt.ylabel("Counts")
    plt.grid(True)
    plt.legend()
    plt.savefig(plot_path)
    print(f"Generated plot: {plot_path}")

    # b) Create a log file with summary statistics
    log_path = output_dir / "full_analysis_log.txt"
    with open(log_path, 'w') as f:
        f.write("---\n")
        f.write("Analysis Log ---\n")
        f.write(f"Input file: {input_file.name}\n")
        f.write(f"Total hits processed: {num_hits}\n")
        f.write(f"Calculated Mean Energy: {np.mean(mock_energy_data):.2f} MeV\n")
        f.write(f"Calculated Std Dev: {np.std(mock_energy_data):.2f} MeV\n")
    print(f"Generated log file: {log_path}")

    # --- 4. Create the Manifest File (The Contract) ---
    manifest_path = output_dir / "results_manifest.json"
    
    # This data would typically be passed in or retrieved from the environment
    # For now, we use placeholders. The CI workflow will provide real values.
    pr_number_placeholder = os.getenv("PR_NUMBER", "123")
    commit_sha_placeholder = os.getenv("COMMIT_SHA", "a1b2c3d4")
    run_size_placeholder = os.getenv("SIM_SIZE", "large")
    job_id_placeholder = os.getenv("JOB_ID", "789123.0")

    manifest_data = {
        "run_metadata": {
            "pr_number": pr_number_placeholder,
            "commit_sha": commit_sha_placeholder,
            "run_size": run_size_placeholder,
            "job_id": job_id_placeholder
        },
        "summary": {
            "title": "Key Metrics from Energy Deposition Analysis",
            "data": {
                "Mean Energy (MeV)": f"{np.mean(mock_energy_data):.2f}",
                "Std Dev (MeV)": f"{np.std(mock_energy_data):.2f}",
                "Total Hits": num_hits
            }
        },
        "artifacts": [
            {
                "path": str(plot_path.name),
                "title": "Energy Deposition Spectrum",
                "description": "Histogram of energy deposited in the scintillator based on mock data.",
                "is_key_result": True
            },
            {
                "path": str(log_path.name),
                "title": "Analysis Log",
                "description": "Full log output from the analysis script, including summary statistics.",
                "is_key_result": False
            }
        ]
    }

    with open(manifest_path, 'w') as f:
        json.dump(manifest_data, f, indent=2)
    
    print(f"Successfully generated manifest file: {manifest_path}")
    print("---\n")
    print("Analysis script finished. ---")

if __name__ == "__main__":
    import os
    main()
