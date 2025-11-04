#!/usr/bin/env python3
"""
Standard validation script for MuonCube simulation output.

This script performs automated validation on simulation output files and generates
standard plots for quality assessment. It reads the data contract to determine
branch names and produces a results manifest listing all generated plots.
"""

import argparse
import json
import os
import sys
import uproot
import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit
from scipy.special import wofz


def landau_distribution(x, mpv, sigma):
    """
    Landau distribution function for fitting energy deposition data.
    """
    # Simplified implementation of the Landau distribution
    # For a full implementation, consider using scipy or a dedicated library
    t = (x - mpv) / sigma
    return np.exp(-0.5 * (t + np.exp(-t)))


def load_data_contract():
    """
    Load the data contract from the simulation config directory.
    """
    contract_path = "simulation/config/data_contract.json"
    if not os.path.exists(contract_path):
        contract_path = os.path.join(os.path.dirname(__file__), "..", "..", "simulation", "config", "data_contract.json")
    
    with open(contract_path, 'r') as f:
        return json.load(f)


def load_data_from_root(file_path, data_contract):
    """
    Load data from the ROOT file using branch names from the data contract.
    """
    with uproot.open(file_path) as file:
        tree = file["events"]  # Assuming the TTree is named "events"
        
        data = {}
        for branch_name in data_contract.keys():
            if branch_name in tree.keys():
                data[branch_name] = tree[branch_name].array(library="np")
            else:
                print(f"Warning: Branch '{branch_name}' not found in ROOT file")
        
        return data


def plot_initial_ke(data, output_dir):
    """
    Plot histogram of initial kinetic energy of primary particles.
    """
    if 'primary_particle_ke' not in data:
        print("Warning: 'primary_particle_ke' not found in data")
        return None
        
    plt.figure(figsize=(10, 6))
    plt.hist(data['primary_particle_ke'], bins=100, alpha=0.7, color='blue', edgecolor='black')
    plt.title('Initial Kinetic Energy Distribution')
    plt.xlabel('Kinetic Energy (MeV)')
    plt.ylabel('Frequency')
    plt.grid(True, alpha=0.3)
    
    output_path = os.path.join(output_dir, 'initial_ke.png')
    plt.savefig(output_path)
    plt.close()
    
    return output_path


def plot_beam_spot(data, output_dir):
    """
    Plot 2D histogram of initial X-Y positions.
    """
    if 'primary_vertex_x' not in data or 'primary_vertex_y' not in data:
        print("Warning: 'primary_vertex_x' or 'primary_vertex_y' not found in data")
        return None
        
    plt.figure(figsize=(10, 8))
    plt.hist2d(data['primary_vertex_x'], data['primary_vertex_y'], bins=100, cmap='Blues')
    plt.colorbar()
    plt.title('Beam Spot: Initial X-Y Position Distribution')
    plt.xlabel('X Position (mm)')
    plt.ylabel('Y Position (mm)')
    plt.grid(True, alpha=0.3)
    
    output_path = os.path.join(output_dir, 'beam_spot.png')
    plt.savefig(output_path)
    plt.close()
    
    return output_path


def plot_initial_direction(data, output_dir):
    """
    Plot histogram of the initial momentum vector's angle.
    """
    if 'primary_particle_px' not in data or 'primary_particle_py' not in data or 'primary_particle_pz' not in data:
        print("Warning: Momentum components not found in data")
        return None
    
    # Calculate angles
    momentum_magnitude = np.sqrt(
        data['primary_particle_px']**2 + 
        data['primary_particle_py']**2 + 
        data['primary_particle_pz']**2
    )
    
    # Calculate polar angle (theta)
    theta = np.arccos(data['primary_particle_pz'] / momentum_magnitude) * 180 / np.pi
    
    plt.figure(figsize=(10, 6))
    plt.hist(theta, bins=100, alpha=0.7, color='green', edgecolor='black')
    plt.title('Initial Momentum Vector Angle Distribution')
    plt.xlabel('Polar Angle (degrees)')
    plt.ylabel('Frequency')
    plt.grid(True, alpha=0.3)
    
    output_path = os.path.join(output_dir, 'initial_direction.png')
    plt.savefig(output_path)
    plt.close()
    
    return output_path


def plot_total_energy_deposition(data, output_dir):
    """
    Plot histogram of total energy deposition with Landau fit.
    """
    if 'total_energy_deposition' not in data:
        print("Warning: 'total_energy_deposition' not found in data")
        return None
        
    plt.figure(figsize=(10, 6))
    counts, bins, patches = plt.hist(data['total_energy_deposition'], bins=100, alpha=0.7, color='red', edgecolor='black', density=True)
    
    # Calculate bin centers
    bin_centers = (bins[:-1] + bins[1:]) / 2
    
    # Initial parameter estimates for Landau fit
    initial_mpv = np.mean(data['total_energy_deposition'])
    initial_sigma = np.std(data['total_energy_deposition'])
    
    try:
        # Fit Landau distribution
        popt, _ = curve_fit(landau_distribution, bin_centers, counts, 
                            p0=[initial_mpv, initial_sigma], maxfev=5000)
        
        # Generate fitted curve
        fitted_y = landau_distribution(bin_centers, *popt)
        plt.plot(bin_centers, fitted_y, 'r-', label=f'Landau fit (MPV={popt[0]:.2f})')
        plt.legend()
    except Exception as e:
        print(f"Warning: Could not fit Landau distribution: {e}")
    
    plt.title('Total Energy Deposition Distribution')
    plt.xlabel('Energy Deposition (MeV)')
    plt.ylabel('Probability Density')
    plt.grid(True, alpha=0.3)
    
    output_path = os.path.join(output_dir, 'total_energy_deposition.png')
    plt.savefig(output_path)
    plt.close()
    
    return output_path


def plot_slab_energy_deposition(data, output_dir):
    """
    Plot a single figure with 4 subplots showing energy deposition in each slab.
    """
    required_slabs = ['slab1_energy', 'slab2_energy', 'slab3_energy', 'slab4_energy']
    
    if not all(slab in data for slab in required_slabs):
        print(f"Warning: Not all slab energy data found in data. Looking for: {required_slabs}")
        return None
    
    fig, axes = plt.subplots(2, 2, figsize=(12, 10))
    fig.suptitle('Energy Deposition in Scintillator Slabs')
    
    slab_names = ['Slab 1', 'Slab 2', 'Slab 3', 'Slab 4']
    
    for i, (slab_key, slab_name) in enumerate(zip(required_slabs, slab_names)):
        ax = axes[i//2, i%2]
        ax.hist(data[slab_key], bins=50, alpha=0.7, edgecolor='black')
        ax.set_title(slab_name)
        ax.set_xlabel('Energy (MeV)')
        ax.set_ylabel('Frequency')
        ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    output_path = os.path.join(output_dir, 'slab_energy_deposition.png')
    plt.savefig(output_path)
    plt.close()
    
    return output_path


def plot_hit_map(data, output_dir):
    """
    Plot 2D heatmap of X-Y hit positions in the scintillators.
    """
    if 'hit_position_x' not in data or 'hit_position_y' not in data:
        print("Warning: 'hit_position_x' or 'hit_position_y' not found in data")
        return None
        
    # Flatten the arrays if they are nested (from events with multiple hits)
    flat_x = np.concatenate(data['hit_position_x']) if data['hit_position_x'].dtype == 'object' else data['hit_position_x']
    flat_y = np.concatenate(data['hit_position_y']) if data['hit_position_y'].dtype == 'object' else data['hit_position_y']
    
    plt.figure(figsize=(10, 8))
    plt.hist2d(flat_x, flat_y, bins=100, cmap='plasma')
    plt.colorbar()
    plt.title('Hit Map: X-Y Position of Detector Hits')
    plt.xlabel('X Position (mm)')
    plt.ylabel('Y Position (mm)')
    plt.grid(True, alpha=0.3)
    
    output_path = os.path.join(output_dir, 'hit_map.png')
    plt.savefig(output_path)
    plt.close()
    
    return output_path


def plot_secondary_particle_census(data, output_dir):
    """
    Create secondary particle census histogram from actual simulation data.
    This function analyzes the secondary_particle_pdg field to count occurrences of different particle types.
    """
    if 'secondary_particle_pdg' not in data:
        print("Warning: 'secondary_particle_pdg' not found in data")
        return None
        
    # Flatten the list of lists to get all secondary particle PDG codes
    all_pdg_codes = []
    for event_pdgs in data['secondary_particle_pdg']:
        if isinstance(event_pdgs, np.ndarray) or isinstance(event_pdgs, list):
            all_pdg_codes.extend(event_pdgs)
        else:
            # Handle the case where it's a scalar value for some reason
            all_pdg_codes.append(event_pdgs)
    
    if not all_pdg_codes:
        print("No secondary particle data to plot")
        return None
    
    # Count occurrences of each PDG code
    unique_pdgs, counts = np.unique(all_pdg_codes, return_counts=True)
    
    # Plot the census
    plt.figure(figsize=(12, 6))
    plt.bar([str(code) for code in unique_pdgs], counts)
    plt.title('Secondary Particle Census')
    plt.xlabel('Particle PDG Code')
    plt.ylabel('Total Count Across All Events')
    plt.xticks(rotation=45)
    plt.grid(True, alpha=0.3)
    
    output_path = os.path.join(output_dir, 'secondary_particle_census.png')
    plt.savefig(output_path, bbox_inches='tight')
    plt.close()
    
    return output_path


def create_results_manifest(plots_list):
    """
    Create a results manifest JSON file listing all generated plots.
    """
    manifest = {
        "validation_script": "standard_validation.py",
        "plots": [plot for plot in plots_list if plot is not None],
        "timestamp": str(np.datetime64('now'))
    }
    
    return manifest


def main():
    parser = argparse.ArgumentParser(description="Standard validation script for MuonCube simulation output")
    parser.add_argument("--input-file", required=True, help="Path to the input .root file")
    parser.add_argument("--output-dir", required=True, help="Directory to save validation plots")
    
    args = parser.parse_args()
    
    # Create output directory if it doesn't exist
    os.makedirs(args.output_dir, exist_ok=True)
    
    # Load data contract
    print("Loading data contract...")
    data_contract = load_data_contract()
    
    # Load data from ROOT file
    print(f"Loading data from {args.input_file}...")
    try:
        data = load_data_from_root(args.input_file, data_contract)
    except Exception as e:
        print(f"Error loading data from ROOT file: {e}")
        sys.exit(1)
    
    # Generate all validation plots
    print("Generating validation plots...")
    plots = []
    
    plots.append(plot_initial_ke(data, args.output_dir))
    plots.append(plot_beam_spot(data, args.output_dir))
    plots.append(plot_initial_direction(data, args.output_dir))
    plots.append(plot_total_energy_deposition(data, args.output_dir))
    plots.append(plot_slab_energy_deposition(data, args.output_dir))
    plots.append(plot_hit_map(data, args.output_dir))
    plots.append(plot_secondary_particle_census(data, args.output_dir))
    
    # Create results manifest
    print("Creating results manifest...")
    manifest = create_results_manifest(plots)
    
    manifest_path = os.path.join(args.output_dir, "results_manifest.json")
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2)
    
    print(f"Validation complete. Results manifest saved to {manifest_path}")
    print(f"Generated {len([p for p in plots if p is not None])} validation plots.")


if __name__ == "__main__":
    main()