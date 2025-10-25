#!/usr/bin/env python3
"""
Simple validation script to check the syntax and basic structure of the analysis script.
"""

import ast
import os

def check_python_syntax(file_path):
    """Check if the Python file has valid syntax."""
    with open(file_path, 'r') as file:
        try:
            ast.parse(file.read())
            print(f"✓ Syntax check passed for {file_path}")
            return True
        except SyntaxError as e:
            print(f"✗ Syntax error in {file_path}: {e}")
            return False

def main():
    script_path = "/home/spring5/MuonCubeSimu/analysis/scripts/validate_muon_angular_distribution.py"
    
    print("Validating analysis script...")
    is_valid = check_python_syntax(script_path)
    
    if is_valid:
        print("✓ Analysis script has valid Python syntax")
    else:
        print("✗ Analysis script has syntax errors")
        return 1
    
    # Also check that it has the required command-line arguments
    with open(script_path, 'r') as f:
        content = f.read()
        if '--input-file' in content and '--output-dir' in content:
            print("✓ Analysis script has required command-line arguments")
        else:
            print("✗ Analysis script missing required command-line arguments")
            return 1
            
        if 'results_manifest.json' in content:
            print("✓ Analysis script generates results manifest")
        else:
            print("✗ Analysis script does not generate results manifest")
            return 1
    
    print("✓ All basic checks passed")
    return 0

if __name__ == "__main__":
    exit(main())