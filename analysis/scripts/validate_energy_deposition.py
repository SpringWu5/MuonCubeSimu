#!/usr/bin/env python3
"""
validate_energy_deposition.py

This script reads the simulation output file (ROOT file) and extracts 
the recorded total energy deposition per event. It prints the energy 
deposition values to the console for validation.
"""

import uproot
import numpy as np
import sys
import os


def validate_energy_deposition(input_file_path):
    """
    Reads the simulation output file and extracts the total energy deposition per event.
    
    Args:
        input_file_path (str): Path to the ROOT file containing simulation output
    """
    
    print(f"Reading simulation output from: {input_file_path}")
    
    # Check if the file exists
    if not os.path.exists(input_file_path):
        print(f"Error: Input file does not exist: {input_file_path}")
        return
    
    try:
        # Open the ROOT file using uproot
        with uproot.open(input_file_path) as file:
            # Get the 'Simu' tree (as defined in OutputManager.cc)
            tree = file["Simu"]
            
            # Check if the totalEnergyDeposition branch exists
            if "totalEnergyDeposition" not in tree.keys():
                print("Error: 'totalEnergyDeposition' branch not found in the tree.")
                print(f"Available branches: {list(tree.keys())}")
                return
            
            # Read the total energy deposition values
            total_energy_deposition = tree["totalEnergyDeposition"].array()
            
            # Read event IDs for reference
            event_ids = tree["eventID"].array()
            
            print(f"Found {len(total_energy_deposition)} events in the simulation output.")
            print("\nEvent ID | Total Energy Deposition (GeV)")
            print("-" * 40)
            
            # Print the energy deposition for each event
            for i, (event_id, energy) in enumerate(zip(event_ids, total_energy_deposition)):
                print(f"{int(event_id):8d} | {energy:22.6f} GeV")
            
            print(f"\nSummary:")
            print(f"Total events processed: {len(total_energy_deposition)}")
            print(f"Min energy deposition: {np.min(total_energy_deposition):.6f} GeV")
            print(f"Max energy deposition: {np.max(total_energy_deposition):.6f} GeV")
            print(f"Mean energy deposition: {np.mean(total_energy_deposition):.6f} GeV")
            print(f"Std energy deposition: {np.std(total_energy_deposition):.6f} GeV")
            
    except Exception as e:
        print(f"Error reading the ROOT file: {str(e)}")
        return


def main():
    """Main function to run the validation script."""
    
    # Default input file path
    default_input_file = "output.root"
    
    # Check command line arguments
    if len(sys.argv) > 1:
        input_file = sys.argv[1]
    else:
        input_file = default_input_file
        print(f"No input file specified. Looking for default file: {default_input_file}")
    
    # Validate the energy deposition
    validate_energy_deposition(input_file)


if __name__ == "__main__":
    main()