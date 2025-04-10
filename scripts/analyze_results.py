#!/usr/bin/env python3

import os
import re
import glob
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import sys
from scipy.interpolate import make_interp_spline
from concurrent.futures import ThreadPoolExecutor, as_completed

# Try to import ROOT
try:
    import ROOT
    has_root = True
    print("PyROOT found - will use ROOT files if available")
except ImportError:
    print("PyROOT not found - will use text files for analysis")
    has_root = False

# Path to the results directory
RESULTS_DIR = "results"

def extract_energy_from_dirname(dirname):
    """Extract energy value (in MeV) from directory name like 'energy_500MeV'"""
    match = re.search(r'energy_(\d+)MeV', os.path.basename(dirname))
    if match:
        return float(match.group(1))
    return None

def extract_data_from_root(root_file):
    """Extract average and peak current from ROOT file"""
    if not has_root:
        return None, None
    
    try:
        # Open the ROOT file
        f = ROOT.TFile(root_file)
        
        # Check if file is open and valid
        if not f or f.IsZombie():
            return None, None
        
        # Try to find the tree with common names
        tree = None
        for name in ["LCData", "NTuple", "lcdata", "ntuple"]:
            tree = f.Get(name)
            if tree and tree.GetEntries() > 0:
                break
        
        if not tree:
            return None, None
        
        # Get branch names
        branches = [b.GetName() for b in tree.GetListOfBranches()]
        
        # Find current branches
        avg_branch = None
        peak_branch = None
        
        for branch in ["AvgCurrent", "Average_Current", "Avg", "avgcurrent"]:
            if branch in branches:
                avg_branch = branch
                break
        
        for branch in ["PeakCurrent", "Peak_Current", "Peak", "peakcurrent"]:
            if branch in branches:
                peak_branch = branch
                break
        
        if not avg_branch or not peak_branch:
            return None, None
        
        # Create temporary histograms for faster data extraction
        h_avg = ROOT.TH1F("h_avg", "avg", 1000, 0, 1000)
        h_peak = ROOT.TH1F("h_peak", "peak", 1000, 0, 1000)
        
        tree.Draw(f"{avg_branch}>>h_avg", "", "goff")
        tree.Draw(f"{peak_branch}>>h_peak", "", "goff")
        
        avg_current = h_avg.GetMean()
        peak_current = h_peak.GetMean()
        
        f.Close()
        return avg_current, peak_current
    
    except Exception:
        if 'f' in locals() and f:
            f.Close()
        return None, None

def extract_data_from_report(report_file):
    """Extract average and peak current from electrometer report file"""
    try:
        with open(report_file, 'r') as f:
            content = f.read()
        
        # Precompiled patterns
        avg_pattern = re.compile(r'Electrometer current:[^,]* ([0-9.e+-]+) pA \(avg\)')
        peak_pattern = re.compile(r'Electrometer current:[^,]*,[^)]*\) ([0-9.e+-]+) pA \(peak\)')
        
        avg_match = avg_pattern.search(content)
        peak_match = peak_pattern.search(content)
        
        if not avg_match or not peak_match:
            # Try fallback patterns
            avg_pattern = re.compile(r'Average Current[:\s]+([0-9.e+-]+)\s*pA', re.IGNORECASE)
            peak_pattern = re.compile(r'Peak Current[:\s]+([0-9.e+-]+)\s*pA', re.IGNORECASE)
            
            avg_match = avg_pattern.search(content)
            peak_match = peak_pattern.search(content)
        
        avg_current = float(avg_match.group(1)) if avg_match else None
        peak_current = float(peak_match.group(1)) if peak_match else None
        
        return avg_current, peak_current
    except:
        return None, None

def extract_data_from_dat_file(dat_file):
    """Extract average and peak current from electrometer data file"""
    try:
        # Use pandas for fast CSV reading
        data = pd.read_csv(dat_file, sep=r'\s+', comment='#', header=None, engine='c')
        
        if data.shape[1] >= 5:
            avg_current = data.iloc[:, 3].mean()
            peak_current = data.iloc[:, 4].mean()
            return avg_current, peak_current
    except:
        pass
    
    return None, None

def process_energy_directory(dir_path):
    """Process a single energy directory and extract current data"""
    energy = extract_energy_from_dirname(dir_path)
    if energy is None:
        return None, None, None
    
    avg_current = None
    peak_current = None
    
    # Try ROOT files first
    if has_root:
        root_files = glob.glob(os.path.join(dir_path, "*.root"))
        if root_files:
            avg_current, peak_current = extract_data_from_root(root_files[0])
    
    # Try report files if ROOT failed
    if avg_current is None or peak_current is None:
        report_files = glob.glob(os.path.join(dir_path, "*_electrometer_report.txt"))
        if report_files:
            avg_current, peak_current = extract_data_from_report(report_files[0])
    
    # Try DAT files if still no data
    if avg_current is None or peak_current is None:
        dat_files = glob.glob(os.path.join(dir_path, "*_electrometer.dat"))
        if dat_files:
            avg_current, peak_current = extract_data_from_dat_file(dat_files[0])
    
    return energy, avg_current, peak_current

def generate_sample_data(energy_points=12):
    """Generate sample data for demonstration if no real data is found"""
    print("No real data found. Generating sample data for demonstration...")
    energies = np.array([13, 25, 50, 100, 250, 500, 1000, 2000, 5000, 10000, 15000, 20000])
    avg_currents = 0.5 * np.log10(energies) + np.random.normal(0, 0.1, len(energies))
    peak_currents = 2.0 * np.log10(energies) + np.random.normal(0, 0.2, len(energies))
    avg_currents = np.maximum(avg_currents, 0.01)
    peak_currents = np.maximum(peak_currents, 0.05)
    return energies, avg_currents, peak_currents

def main():
    print("Analyzing 5CB Liquid Crystal Detector simulation results...")
    
    # Find all energy directories
    energy_dirs = glob.glob(os.path.join(RESULTS_DIR, "energy_*MeV"))
    
    if not energy_dirs:
        print(f"No energy directories found in {RESULTS_DIR}")
        choice = input("Would you like to generate sample data for demonstration? (y/n): ")
        if choice.lower() == 'y':
            energies, avg_currents, peak_currents = generate_sample_data()
            use_sample_data = True
        else:
            print("Exiting without generating plots.")
            return
    else:
        use_sample_data = False
        
        # Process directories in parallel
        results = []
        # Adjust max_workers based on CPU cores, but limit to avoid memory issues
        max_workers = min(16, os.cpu_count() or 4)
        
        print(f"Processing {len(energy_dirs)} energy directories using {max_workers} workers...")
        
        with ThreadPoolExecutor(max_workers=max_workers) as executor:
            future_to_dir = {executor.submit(process_energy_directory, dir_path): dir_path for dir_path in energy_dirs}
            
            # Process results as they complete
            for i, future in enumerate(as_completed(future_to_dir)):
                dir_path = future_to_dir[future]
                try:
                    energy, avg_current, peak_current = future.result()
                    if energy is not None and avg_current is not None and peak_current is not None:
                        results.append((energy, avg_current, peak_current))
                        if i % 50 == 0 or i == len(energy_dirs) - 1:
                            print(f"Processed {i+1}/{len(energy_dirs)} directories")
                except Exception as e:
                    print(f"Error processing {dir_path}: {e}")
        
        print(f"Successfully extracted data from {len(results)} out of {len(energy_dirs)} directories")
        
        if not results:
            print("\nNo valid data found in the energy directories")
            choice = input("Would you like to generate sample data for demonstration? (y/n): ")
            if choice.lower() == 'y':
                energies, avg_currents, peak_currents = generate_sample_data()
                use_sample_data = True
            else:
                print("Exiting without generating plots.")
                return
        else:
            # Sort results by energy
            results.sort(key=lambda x: x[0])
            energies = np.array([r[0] for r in results])
            avg_currents = np.array([r[1] for r in results])
            peak_currents = np.array([r[2] for r in results])
    
    # Create a DataFrame for easy data manipulation
    df = pd.DataFrame({
        'Energy (MeV)': energies,
        'Average Current (pA)': avg_currents,
        'Peak Current (pA)': peak_currents
    })
    
    # Save raw data to CSV
    plots_dir = os.path.join(RESULTS_DIR, "plots")
    os.makedirs(plots_dir, exist_ok=True)
    csv_file = os.path.join(RESULTS_DIR, "current_vs_energy_raw.csv")
    df.to_csv(csv_file, index=False, float_format='%.6f')
    print(f"Raw data saved to {csv_file}")
    
    # Common x-axis ticks
    tick_energies = [10, 50, 100, 500, 1000, 5000, 10000, 20000]
    # Filter to keep only ticks within the data range
    tick_energies = [e for e in tick_energies if e >= min(energies) and e <= max(energies)]
    
    # Set up a non-interactive backend for faster plotting
    plt.switch_backend('Agg')
    
    # Plot Average Current vs Energy
    plt.figure(figsize=(14, 10))
    
    # With many points, only use the line without individual markers
    plt.plot(energies, avg_currents, '-', color='darkblue', linewidth=2, label='Average Current')
    
    plt.xscale('log')
    plt.xlabel('Beam Energy (MeV)', fontsize=16)
    plt.ylabel('Average Current (pA)', fontsize=16)
    title = 'Average Current vs Beam Energy in 5CB Liquid Crystal Detector'
    if use_sample_data:
        title += ' (SAMPLE DATA)'
    plt.title(title, fontsize=18)
    plt.grid(True, which="both", ls="--", alpha=0.7)
    
    # Improved tick formatting
    plt.xticks(tick_energies, [f"{e} MeV" if e < 1000 else f"{e/1000:.0f} GeV" for e in tick_energies], fontsize=12)
    plt.yticks(fontsize=12)
    
    # Set format for y-axis to show more decimal places
    plt.gca().yaxis.set_major_formatter(plt.matplotlib.ticker.FormatStrFormatter('%.6f'))
    
    plt.tight_layout()
    
    # Save the figure
    avg_plot_file = os.path.join(plots_dir, "average_current_vs_energy.png")
    plt.savefig(avg_plot_file, dpi=300)
    print(f"Average current plot saved to {avg_plot_file}")
    plt.close()
    
    # Peak Current Plot
    plt.figure(figsize=(14, 10))
    plt.plot(energies, peak_currents, '-', color='darkred', linewidth=2, label='Peak Current')
    
    plt.xscale('log')
    plt.xlabel('Beam Energy (MeV)', fontsize=16)
    plt.ylabel('Peak Current (pA)', fontsize=16)
    title = 'Peak Current vs Beam Energy in 5CB Liquid Crystal Detector'
    if use_sample_data:
        title += ' (SAMPLE DATA)'
    plt.title(title, fontsize=18)
    plt.grid(True, which="both", ls="--", alpha=0.7)
    plt.xticks(tick_energies, [f"{e} MeV" if e < 1000 else f"{e/1000:.0f} GeV" for e in tick_energies], fontsize=12)
    plt.yticks(fontsize=12)
    
    # Set format for y-axis to show more decimal places
    plt.gca().yaxis.set_major_formatter(plt.matplotlib.ticker.FormatStrFormatter('%.6f'))
    
    plt.tight_layout()
    
    peak_plot_file = os.path.join(plots_dir, "peak_current_vs_energy.png")
    plt.savefig(peak_plot_file, dpi=300)
    print(f"Peak current plot saved to {peak_plot_file}")
    plt.close()
    
    # Combined Plot
    plt.figure(figsize=(14, 10))
    plt.plot(energies, avg_currents, '-', color='blue', linewidth=2, label='Average Current')
    plt.plot(energies, peak_currents, '-', color='red', linewidth=2, label='Peak Current')
    plt.xscale('log')
    plt.xlabel('Beam Energy (MeV)', fontsize=16)
    plt.ylabel('Current (pA)', fontsize=16)
    title = 'Current vs Beam Energy in 5CB Liquid Crystal Detector'
    if use_sample_data:
        title += ' (SAMPLE DATA)'
    plt.title(title, fontsize=18)
    plt.grid(True, which="both", ls="--", alpha=0.7)
    plt.legend(fontsize=14)
    plt.xticks(tick_energies, [f"{e} MeV" if e < 1000 else f"{e/1000:.0f} GeV" for e in tick_energies], fontsize=12)
    plt.yticks(fontsize=12)
    
    # Set format for y-axis to show more decimal places
    plt.gca().yaxis.set_major_formatter(plt.matplotlib.ticker.FormatStrFormatter('%.6f'))
    
    plt.tight_layout()
    
    combined_plot_file = os.path.join(plots_dir, "current_vs_energy_combined.png")
    plt.savefig(combined_plot_file, dpi=300)
    print(f"Combined plot saved to {combined_plot_file}")
    plt.close()
    
    # Current Ratio Plot
    plt.figure(figsize=(14, 10))
    current_ratio = peak_currents / avg_currents
    plt.plot(energies, current_ratio, '-', color='purple', linewidth=2)
    plt.xscale('log')
    plt.xlabel('Beam Energy (MeV)', fontsize=16)
    plt.ylabel('Peak/Average Current Ratio', fontsize=16)
    title = 'Current Ratio vs Beam Energy in 5CB Liquid Crystal Detector'
    if use_sample_data:
        title += ' (SAMPLE DATA)'
    plt.title(title, fontsize=18)
    plt.grid(True, which="both", ls="--", alpha=0.7)
    plt.xticks(tick_energies, [f"{e} MeV" if e < 1000 else f"{e/1000:.0f} GeV" for e in tick_energies], fontsize=12)
    plt.yticks(fontsize=12)
    
    # Set format for y-axis to show more decimal places
    plt.gca().yaxis.set_major_formatter(plt.matplotlib.ticker.FormatStrFormatter('%.6f'))
    
    plt.tight_layout()
    
    ratio_plot_file = os.path.join(plots_dir, "current_ratio_vs_energy.png")
    plt.savefig(ratio_plot_file, dpi=300)
    print(f"Current ratio plot saved to {ratio_plot_file}")
    plt.close()
    
    print("\nAnalysis complete!")
    print(f"Processed {len(energies)} unique energy points")
    print(f"Energy range: {min(energies):.1f} MeV to {max(energies):.1f} MeV")
    if use_sample_data:
        print("NOTE: Plots were created using SAMPLE DATA, not actual simulation results!")

if __name__ == "__main__":
    main()
