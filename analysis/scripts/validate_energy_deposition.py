#!/usr/bin/env python3
"""
Script to validate muon energy deposition recording in the simulation.

This script reads the simulation output file (ROOT format) and extracts 
the recorded energy deposition for validation.
"""

import argparse
import sys
try:
    import ROOT
except ImportError:
    print("Error: ROOT Python bindings not found. Please install ROOT or set up the environment.")
    sys.exit(1)


def validate_energy_deposition(input_file):
    """
    Read the simulation output file and extract energy deposition data.
    
    Args:
        input_file (str): Path to the simulation ROOT output file
    
    Returns:
        list: List of energy deposition values per event
    """
    print(f"Opening file: {input_file}")
    
    # Open the ROOT file
    root_file = ROOT.TFile.Open(input_file)
    if not root_file or root_file.IsZombie():
        print(f"Error: Could not open file {input_file}")
        return []
    
    # Access the simulation tree
    tree = root_file.Get("Simu")
    if not tree:
        print("Error: Could not find 'Simu' tree in the file")
        root_file.Close()
        return []
    
    print(f"Successfully opened tree with {tree.GetEntries()} entries")
    
    # List to store energy deposition values
    energy_depositions = []
    
    # Process all events in the tree
    for entry_idx in range(tree.GetEntries()):
        tree.GetEntry(entry_idx)
        
        # Extract event ID and total energy deposition
        event_id = tree.eventID
        total_energy = tree.totalEnergyDeposition
        
        print(f"Event {event_id}: Total energy deposited = {total_energy:.6f} GeV")
        
        energy_depositions.append(total_energy)
    
    root_file.Close()
    return energy_depositions


def main():
    parser = argparse.ArgumentParser(description="Validate muon energy deposition from simulation output")
    parser.add_argument("--input-file", required=True, help="Input ROOT file from simulation")
    parser.add_argument("--output-file", help="Optional output file to save results summary")
    
    args = parser.parse_args()
    
    # Validate the energy deposition
    energies = validate_energy_deposition(args.input_file)
    
    if energies:
        print(f"\nSummary:")
        print(f"Total events processed: {len(energies)}")
        print(f"Total energy deposited across all events: {sum(energies):.6f} GeV")
        print(f"Average energy per event: {sum(energies)/len(energies):.6f} GeV" if energies else "N/A")
        print(f"Max energy in single event: {max(energies) if energies else 0:.6f} GeV")
        print(f"Min energy in single event: {min(energies) if energies else 0:.6f} GeV")
        
        # Optionally save results to a file
        if args.output_file:
            with open(args.output_file, 'w') as f:
                f.write("EventID,EnergyDeposition_GeV\n")
                for i, energy in enumerate(energies):
                    f.write(f"{i},{energy:.8f}\n")
            print(f"\nResults saved to {args.output_file}")
    else:
        print("No energy deposition data found.")


if __name__ == "__main__":
    main()