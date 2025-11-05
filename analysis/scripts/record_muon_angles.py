#!/usr/bin/env python3
"""
Script to record and analyze muon incident angles from simulation output.

This script reads the initial_theta and initial_phi branches from a ROOT file
and generates distribution plots for these angles.
"""

import argparse
import uproot
import numpy as np
import matplotlib.pyplot as plt
import json
import os


def load_data(file_path):
    """
    Load the initial theta and phi angles from the ROOT file.
    
    Args:
        file_path (str): Path to the ROOT file to analyze
        
    Returns:
        tuple: Arrays of theta and phi values
    """
    print(f"Loading data from {file_path}")
    
    with uproot.open(file_path) as file:
        tree = file["events"]  # Assuming the tree is named "events"
        
        # Extract the angle data
        initial_theta = tree["initial_theta"].array(library="np")
        initial_phi = tree["initial_phi"].array(library="np")
        
        print(f"Loaded {len(initial_theta)} events")
        return initial_theta, initial_phi


def create_plots(initial_theta, initial_phi, output_dir):
    """
    Create plots for the angle distributions.
    
    Args:
        initial_theta (array): Array of theta angle values
        initial_phi (array): Array of phi angle values
        output_dir (str): Directory to save the plots
    """
    os.makedirs(output_dir, exist_ok=True)
    
    # Plot 1: Distribution of initial theta angles
    plt.figure(figsize=(10, 6))
    plt.hist(initial_theta, bins=100, alpha=0.7, color='blue', edgecolor='black')
    plt.xlabel('Theta (radians)')
    plt.ylabel('Frequency')
    plt.title('Distribution of Initial Muon Polar Angles (Theta)')
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    theta_hist_path = os.path.join(output_dir, "initial_theta_distribution.png")
    plt.savefig(theta_hist_path)
    plt.close()
    print(f"Saved theta distribution plot to {theta_hist_path}")

    # Plot 2: Distribution of initial phi angles
    plt.figure(figsize=(10, 6))
    plt.hist(initial_phi, bins=100, alpha=0.7, color='green', edgecolor='black')
    plt.xlabel('Phi (radians)')
    plt.ylabel('Frequency')
    plt.title('Distribution of Initial Muon Azimuthal Angles (Phi)')
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    phi_hist_path = os.path.join(output_dir, "initial_phi_distribution.png")
    plt.savefig(phi_hist_path)
    plt.close()
    print(f"Saved phi distribution plot to {phi_hist_path}")

    # Plot 3: 2D histogram of theta vs phi
    plt.figure(figsize=(10, 8))
    plt.hist2d(initial_theta, initial_phi, bins=100, cmap='Blues')
    plt.colorbar(label='Frequency')
    plt.xlabel('Theta (radians)')
    plt.ylabel('Phi (radians)')
    plt.title('2D Distribution of Initial Muon Angles (Theta vs Phi)')
    plt.tight_layout()
    angle_2d_path = os.path.join(output_dir, "2d_angle_distribution.png")
    plt.savefig(angle_2d_path)
    plt.close()
    print(f"Saved 2D angle distribution plot to {angle_2d_path}")

    return [theta_hist_path, phi_hist_path, angle_2d_path]


def generate_results_manifest(plot_files, output_dir):
    """
    Generate a results manifest JSON file.
    
    Args:
        plot_files (list): List of paths to generated plot files
        output_dir (str): Directory where manifest should be saved
    """
    manifest = {
        "task": "record_muon_angles",
        "plots": [
            {"name": "initial_theta_distribution", "path": os.path.relpath(plot_files[0], output_dir)},
            {"name": "initial_phi_distribution", "path": os.path.relpath(plot_files[1], output_dir)},
            {"name": "2d_angle_distribution", "path": os.path.relpath(plot_files[2], output_dir)}
        ],
        "generated_at": str(np.datetime64('now'))
    }

    manifest_path = os.path.join(output_dir, "results_manifest.json")
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2)
    
    print(f"Generated results manifest at {manifest_path}")
    return manifest_path


def main():
    parser = argparse.ArgumentParser(description="Record and analyze muon incident angles from simulation output")
    parser.add_argument("--input-file", required=True, help="Path to the input .root file")
    parser.add_argument("--output-dir", required=True, help="Directory to save output plots and manifest")
    
    args = parser.parse_args()
    
    # Ensure the output directory exists
    os.makedirs(args.output_dir, exist_ok=True)
    
    try:
        # Load the data
        initial_theta, initial_phi = load_data(args.input_file)
        
        # Create the plots
        plot_files = create_plots(initial_theta, initial_phi, args.output_dir)
        
        # Generate results manifest
        generate_results_manifest(plot_files, args.output_dir)
        
        print("Angle recording and analysis completed successfully!")
        
    except FileNotFoundError:
        print(f"Error: Input file '{args.input_file}' not found.")
        return 1
    except Exception as e:
        print(f"Error during analysis: {str(e)}")
        return 1
    
    return 0


if __name__ == "__main__":
    exit(main())