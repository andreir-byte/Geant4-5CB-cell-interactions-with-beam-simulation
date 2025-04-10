#!/bin/bash

# Configuration - CHANGE THIS to the path of your simulation executable
SIM_EXECUTABLE="/home/jorge/Sims/LCDetector/build/LCDetector"
SIM_DIR=$(dirname "$SIM_EXECUTABLE")

# Check if the executable exists
if [ ! -f "$SIM_EXECUTABLE" ]; then
    echo "Error: Simulation executable not found at $SIM_EXECUTABLE"
    echo "Please edit this script and set the correct path in the SIM_EXECUTABLE variable."
    exit 1
fi

# Create results directory (use absolute path)
RESULTS_DIR="$(pwd)/results"
mkdir -p "$RESULTS_DIR"

# Default settings
MIN_ENERGY=10    # Starting energy in MeV
MAX_ENERGY=20000   # Maximum energy in MeV
NUM_EVENTS=1000000  # Number of events per simulation
PARTICLE_TYPE="positron"  # Default particle type
MAX_PARALLEL_JOBS=3  # Maximum number of parallel simulations

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --min)
            MIN_ENERGY="$2"
            shift 2
            ;;
        --max)
            MAX_ENERGY="$2"
            shift 2
            ;;
        --events)
            NUM_EVENTS="$2"
            shift 2
            ;;
        --particle)
            PARTICLE_TYPE="$2"
            shift 2
            ;;
        --parallel)
            MAX_PARALLEL_JOBS="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --min VALUE        Minimum energy in MeV (default: 2160)"
            echo "  --max VALUE        Maximum energy in MeV (default: 20000)"
            echo "  --events VALUE     Number of events per energy (default: 1000000)"
            echo "  --particle TYPE    Particle type (default: proton)"
            echo "  --parallel N       Maximum parallel jobs (default: 3)"
            echo "  --help             Display this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo "========================================================"
echo "5CB Liquid Crystal Detector Energy Sweep (Multiples of 10)"
echo "========================================================"
echo "Minimum energy: $MIN_ENERGY MeV"
echo "Maximum energy: $MAX_ENERGY MeV"
echo "Events per energy: $NUM_EVENTS"
echo "Particle type: $PARTICLE_TYPE"
echo "Maximum parallel jobs: $MAX_PARALLEL_JOBS"
echo "========================================================"

# Generate energy values as multiples of 10, ensuring full range coverage
declare -a ENERGIES=()

# Round MIN_ENERGY to the next multiple of 10
MIN_ENERGY_ROUNDED=$(( (($MIN_ENERGY + 9) / 10) * 10 ))

# Create logarithmically spaced multiples of 10
current_energy=$MIN_ENERGY_ROUNDED
while [ $current_energy -le $MAX_ENERGY ]; do
    ENERGIES+=($current_energy)
    
    # Calculate next energy value using a multiplier approach
    # This gives roughly logarithmic spacing but only using multiples of 10
    if [ $current_energy -lt 100 ]; then
        # For very small energies, increment by 2
        current_energy=$(( $current_energy + 1 ))
    elif [ $current_energy -lt 1000 ]; then
        # For energies under 1 GeV, increment by 0.5
        current_energy=$(( $current_energy + 10 ))
    elif [ $current_energy -lt 10000 ]; then
        # For energies 1-10 GeV, increment by 1000
        current_energy=$(( $current_energy + 100 ))
    else
        # For energies over 10 GeV, increment by 2000
        current_energy=$(( $current_energy + 500 ))
    fi
done

# Add the exact MAX_ENERGY if it's not already included
if [ ${ENERGIES[-1]} -ne $MAX_ENERGY ]; then
    ENERGIES+=($MAX_ENERGY)
fi

echo "Energy points to simulate (MeV) [${#ENERGIES[@]} points]:"
for energy in "${ENERGIES[@]}"; do
    if [ $energy -ge 1000 ]; then
        echo "  - $energy MeV ($(echo "scale=1; $energy/1000" | bc -l) GeV)"
    else
        echo "  - $energy MeV"
    fi
done

echo ""
read -p "Proceed with these settings? (y/n): " CONFIRM
if [[ $CONFIRM != "y" && $CONFIRM != "Y" ]]; then
    echo "Simulation aborted by user."
    exit 0
fi

# Function to run simulation at specified energy
run_simulation() {
    local energy=$1
    local energy_dir="${RESULTS_DIR}/energy_${energy}MeV"
    
    # Check if directory already exists (result may already exist)
    if [ -d "$energy_dir" ] && [ -f "${energy_dir}/LC_${PARTICLE_TYPE}_${energy}MeV.root" ]; then
        echo "Simulation for $energy MeV already exists, skipping."
        return
    fi
    
    # Create directory for this energy
    mkdir -p "$energy_dir"
    
    # Create energy-specific macro file
    local energy_macro="${SIM_DIR}/run_${energy}MeV.mac"
    cat > "$energy_macro" << EOF
# Set parameters BEFORE starting the run
/LC/beam/particle $PARTICLE_TYPE
/LC/beam/energy $energy MeV
# Now start the run with the correct parameters
/run/initialize
/run/beamOn $NUM_EVENTS
EOF
    
    # Run the simulation from its original directory
    cd "$SIM_DIR"
    
    # Run the simulation with minimal output
    ./$(basename "$SIM_EXECUTABLE") "run_${energy}MeV.mac" > "${energy_dir}/simulation_output.log" 2>&1
    
    # Check if ROOT file was created
    local root_file="LC_${PARTICLE_TYPE}_${energy}MeV.root"
    
    if [ -f "$root_file" ] && [ -s "$root_file" ]; then
        # Copy output files
        cp "LC_${PARTICLE_TYPE}_${energy}MeV"* "$energy_dir/"
        
        # Clean up original files
        rm -f "LC_${PARTICLE_TYPE}_${energy}MeV"*
    else
        echo "ERROR: ROOT file was not created for $energy MeV!"
    fi
    
    # Clean up the macro file
    rm -f "run_${energy}MeV.mac"
    
    # Return to the original directory
    cd - > /dev/null
    
    echo "Completed simulation for $energy MeV"
}

# Parallel execution handler
run_parallel_simulations() {
    local PIDS=()
    local ENERGIES=("$@")
    local TOTAL=${#ENERGIES[@]}
    local COMPLETED=0
    
    # Process energies in batches
    for ((i=0; i<${#ENERGIES[@]}; i+=$MAX_PARALLEL_JOBS)); do
        local BATCH=()
        
        # Create a batch of jobs
        for ((j=0; j<$MAX_PARALLEL_JOBS && i+j<${#ENERGIES[@]}; j++)); do
            BATCH+=("${ENERGIES[i+j]}")
        done
        
        # Launch jobs in parallel
        PIDS=()
        for energy in "${BATCH[@]}"; do
            echo "Starting simulation for $energy MeV ($(($COMPLETED+1))/$TOTAL)"
            run_simulation "$energy" &
            PIDS+=($!)
            COMPLETED=$((COMPLETED+1))
        done
        
        # Wait for all jobs in this batch to complete
        for pid in "${PIDS[@]}"; do
            wait $pid
        done
    done
}

# Run simulations in parallel
start_time=$(date +%s)
run_parallel_simulations "${ENERGIES[@]}"
end_time=$(date +%s)
total_time=$((end_time - start_time))

echo "========================================================"
echo "All simulations completed"
echo "Results saved in: $RESULTS_DIR"
echo "Total execution time: $total_time seconds"
echo "========================================================"

# Create a summary file
SUMMARY_FILE="${RESULTS_DIR}/simulation_summary.txt"
cat > "$SUMMARY_FILE" << EOF
5CB Liquid Crystal Detector Simulation Summary
=============================================
Date: $(date)
Particle type: $PARTICLE_TYPE
Events per energy: $NUM_EVENTS
Total energies simulated: ${#ENERGIES[@]}
Energy range: $MIN_ENERGY MeV to $MAX_ENERGY MeV
Total execution time: $total_time seconds
Parallel jobs: $MAX_PARALLEL_JOBS

Simulated energies (MeV):
$(printf "%s\n" "${ENERGIES[@]}")

To analyze these results, run:
./analyze_results.py
EOF

# Save all energies to a separate file for reference
ENERGIES_FILE="${RESULTS_DIR}/energy_list.txt"
printf "%s\n" "${ENERGIES[@]}" > "$ENERGIES_FILE"

echo "Summary saved to: $SUMMARY_FILE"
echo "========================================================"
echo "To analyze the results, run: ./analyze_results.py"
echo "========================================================"
