#!/usr/bin/env python3
"""
Analysis script to validate the energy deposition spectrum of high-energy muon beam (10-40 GeV).
This script reads the simulation output file and generates a histogram of energy deposition.
"""

import argparse
import json
import os
import numpy as np
import matplotlib
matplotlib.use('Agg')  # Use non-interactive backend
import matplotlib.pyplot as plt
import uproot


def main():
    parser = argparse.ArgumentParser(description="Validate energy deposition of high-energy muon beam")
    parser.add_argument("--input-file", required=True, help="Path to the simulation's ROOT output file")
    parser.add_argument("--output-dir", required=True, help="Directory to save all outputs (plots, data files, logs)")
    
    args = parser.parse_args()
    
    # Ensure output directory exists
    os.makedirs(args.output_dir, exist_ok=True)
    
    # Read the ROOT file using uproot
    with uproot.open(args.input_file) as file:
        # Extract the 'events' TTree (assuming it contains 'totalEdep' branch)
        events_tree = file["events"]
        
        # Extract the 'totalEdep' branch into a numpy array
        total_edep = events_tree["totalEdep"].array(library="np")
    
    # Create histogram of the totalEdep data
    plt.figure(figsize=(10, 7))
    plt.hist(total_edep, bins=50, edgecolor='black', alpha=0.7)
    plt.title("Energy Deposition of 10-40 GeV Muons")
    plt.xlabel("Energy Deposited [MeV]")
    plt.ylabel("Counts")
    plt.grid(True, linestyle='--', alpha=0.6)
    
    # Save the plot to a file named energy_deposition_spectrum.png inside the output directory
    plot_path = os.path.join(args.output_dir, "energy_deposition_spectrum.png")
    plt.savefig(plot_path)
    plt.close()
    
    # Calculate summary data
    mean_edep = float(np.mean(total_edep))
    std_dev_edep = float(np.std(total_edep))
    
    # Generate results_manifest.json as final step
    manifest = {
        "plot_files": ["energy_deposition_spectrum.png"],
        "summary_data": {
            "mean_edep": mean_edep,
            "std_dev_edep": std_dev_edep
        }
    }
    
    manifest_path = os.path.join(args.output_dir, "results_manifest.json")
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2)
    
    print(f"Analysis complete. Output saved to {args.output_dir}")
    print(f"Mean energy deposition: {mean_edep:.2f} MeV")
    print(f"Standard deviation: {std_dev_edep:.2f} MeV")


if __name__ == "__main__":
    main()