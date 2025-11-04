#!/usr/bin/env python3
"""
Analysis script to validate the energy deposition spectrum for mono-energetic muon beam.
This script reads the simulation output file and generates a histogram of energy deposition.
According to the Data Contract: TTree 'Simu', branch 'totalEnergyDeposition'.
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
    parser = argparse.ArgumentParser(description="Validate energy deposition of mono-energetic muon beam")
    parser.add_argument("--input-file", required=True, help="Path to the simulation's ROOT output file")
    parser.add_argument("--output-dir", required=True, help="Directory to save all outputs (plots, data files, logs)")
    
    args = parser.parse_args()
    
    # Ensure output directory exists
    os.makedirs(args.output_dir, exist_ok=True)
    
    # Read the ROOT file using uproot
    with uproot.open(args.input_file) as file:
        # Extract the 'Simu' TTree (as per Data Contract)
        simu_tree = file["Simu"]
        
        # Extract the 'totalEnergyDeposition' branch into a numpy array (as per Data Contract)
        total_edep = simu_tree["totalEnergyDeposition"].array(library="np")
    
    # Create histogram of the total energy deposition data
    plt.figure(figsize=(10, 7))
    plt.hist(total_edep, bins=50, edgecolor='black', alpha=0.7)
    plt.title("Energy Deposition of Mono-Energetic Muon Beam (150 MeV)")
    plt.xlabel("Total Energy Deposited [MeV]")
    plt.ylabel("Counts")
    plt.grid(True, linestyle='--', alpha=0.6)
    
    # Add a text box with statistics
    mean_val = np.mean(total_edep)
    std_val = np.std(total_edep)
    textstr = f'Mean: {mean_val:.2f} MeV\nStd: {std_val:.2f} MeV'
    props = dict(boxstyle='round', facecolor='wheat', alpha=0.5)
    plt.text(0.7, 0.95, textstr, transform=plt.gca().transAxes, fontsize=10,
             verticalalignment='top', bbox=props)
    
    # Save the plot to a file named energy_deposition_spectrum.png inside the output directory
    plot_path = os.path.join(args.output_dir, "energy_deposition_spectrum.png")
    plt.savefig(plot_path)
    plt.close()
    
    # Calculate summary data
    mean_edep = float(mean_val)
    std_dev_edep = float(std_val)
    
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