#!/usr/bin/env python3
"""
Analysis script to validate the angular distribution of primary muons.
This script reads the ROOT output file and creates a histogram of the cosine
of the incident angle for primary muons.
"""

import argparse
import json
import os
import numpy as np
import matplotlib.pyplot as plt
from ROOT import TFile, TTree


def calculate_cos_theta(px, py, pz):
    """
    Calculate the cosine of the angle with the Z-axis: cos_theta = p_z / |p|
    """
    p_magnitude = np.sqrt(px**2 + py**2 + pz**2)
    # Avoid division by zero
    cos_theta = np.divide(pz, p_magnitude, out=np.zeros_like(pz), where=p_magnitude!=0)
    return cos_theta


def main():
    parser = argparse.ArgumentParser(description='Validate muon angular distribution')
    parser.add_argument('--input-file', required=True, help='Input ROOT file path')
    parser.add_argument('--output-dir', required=True, help='Output directory for plots and results')
    
    args = parser.parse_args()
    
    # Ensure output directory exists
    os.makedirs(args.output_dir, exist_ok=True)
    
    # Open the ROOT file
    root_file = TFile.Open(args.input_file)
    if not root_file or root_file.IsZombie():
        raise FileNotFoundError(f"Could not open ROOT file: {args.input_file}")
    
    # Get the tree - looking for the Simu tree that contains the data
    tree = root_file.Get("Simu")
    if not tree:
        raise ValueError("Could not find 'Simu' tree in the ROOT file")
    
    # Check if the initial momentum branches exist
    branches = [key.GetName() for key in tree.GetListOfBranches()]
    print(f"Available branches: {branches}")
    
    # Extract initial momentum components
    init_px_list = []
    init_py_list = []
    init_pz_list = []
    
    # Loop through the tree and get initial momentum values
    for entry in tree:
        # Access the initial momentum stored in McEventRoot
        # These are vectors stored per event
        for px, py, pz in zip(entry.initial_momentum_x, entry.initial_momentum_y, entry.initial_momentum_z):
            init_px_list.append(px)
            init_py_list.append(py)
            init_pz_list.append(pz)
    
    # Calculate cosine of the angle with Z-axis
    if init_px_list and init_py_list and init_pz_list:
        px_array = np.array(init_px_list)
        py_array = np.array(init_py_list)
        pz_array = np.array(init_pz_list)
        
        cos_theta_values = calculate_cos_theta(px_array, py_array, pz_array)
        
        # Create histogram
        plt.figure(figsize=(10, 6))
        plt.hist(cos_theta_values, bins=50, range=(-1, 1), alpha=0.7, edgecolor='black')
        plt.xlabel('cos(θ) - Cosine of incident angle')
        plt.ylabel('Frequency')
        plt.title('Distribution of cos(θ) for Primary Muons')
        plt.grid(True, alpha=0.3)
        
        # Save the plot
        plot_path = os.path.join(args.output_dir, 'muon_costheta_distribution.png')
        plt.savefig(plot_path, dpi=300, bbox_inches='tight')
        plt.close()
        
        # Calculate summary statistics
        mean_cos_theta = np.mean(cos_theta_values) if len(cos_theta_values) > 0 else 0.0
        
        # Create results manifest
        manifest = {
            "summary_data": {
                "mean_cos_theta": float(mean_cos_theta)
            },
            "artifacts": [
                {
                    "file_path": "muon_costheta_distribution.png",
                    "description": "Distribution of the cosine of the incident angle for primary muons.",
                    "is_key_result": True
                }
            ]
        }
        
        # Write manifest file
        manifest_path = os.path.join(args.output_dir, 'results_manifest.json')
        with open(manifest_path, 'w') as manifest_file:
            json.dump(manifest, manifest_file, indent=2)
        
        print(f"Analysis complete. Results saved to {args.output_dir}")
        print(f"Mean cos(θ): {mean_cos_theta:.4f}")
    else:
        print("No initial momentum data found in the tree.")
        
        # Create an empty results manifest
        manifest = {
            "summary_data": {
                "mean_cos_theta": 0.0
            },
            "artifacts": [
                {
                    "file_path": "muon_costheta_distribution.png",
                    "description": "Distribution of the cosine of the incident angle for primary muons.",
                    "is_key_result": True
                }
            ]
        }
        
        # Create an empty plot
        plt.figure(figsize=(10, 6))
        plt.text(0.5, 0.5, 'No data available', horizontalalignment='center', verticalalignment='center', transform=plt.gca().transAxes)
        plt.xlabel('cos(θ) - Cosine of incident angle')
        plt.ylabel('Frequency')
        plt.title('Distribution of cos(θ) for Primary Muons')
        plot_path = os.path.join(args.output_dir, 'muon_costheta_distribution.png')
        plt.savefig(plot_path, dpi=300, bbox_inches='tight')
        plt.close()
        
        # Write empty manifest file
        manifest_path = os.path.join(args.output_dir, 'results_manifest.json')
        with open(manifest_path, 'w') as manifest_file:
            json.dump(manifest, manifest_file, indent=2)


if __name__ == "__main__":
    main()