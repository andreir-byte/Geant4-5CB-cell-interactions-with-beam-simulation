# 5CB Liquid Crystal Detector Simulation

A Geant4-based simulation of a 5CB liquid crystal detector with electrometer current measurement capabilities. This simulation models the response of a thin liquid crystal film to various types of radiation and provides detailed analysis of charge collection and current generation.

## Overview

This simulation models a 15mm × 25mm × 100μm 5CB liquid crystal detector with:
- ITO glass electrodes
- Variable bias voltage (creating an electric field across the detector)
- Electrometer connections for current measurement
- Particle tracking with ionization simulation
- Charge pair creation and collection
- Detailed electrometer response modeling

## Requirements

### System Requirements
- Linux operating system (optimized for Linux)
- GCC compiler with C++17 support
- CMake 3.16 or higher
- 2GB RAM minimum (4GB recommended for large runs)
- Multi-core CPU recommended (simulation uses 12 threads by default)

### Software Dependencies
- Geant4 11.0 or later with the following components:
  - UI and visualization support
  - Multithreading support
  - Qt support (optional, for visualization)
  - OpenGL (optional, for visualization)
- ROOT data analysis framework (optional, for analyzing output)

## Installation

### Installing Geant4 (if not already installed)

```bash
# Install prerequisites
sudo apt-get update
sudo apt-get install -y build-essential \
  cmake \
  libxerces-c-dev \
  libxmu-dev \
  libmotif-dev \
  libgl1-mesa-dev \
  libglu1-mesa-dev \
  libxt-dev \
  qt5-default \
  libqt5opengl5-dev

# Download Geant4 (replace with your desired version)
wget http://geant4-data.web.cern.ch/geant4-data/releases/geant4-v11.0.0.tar.gz
tar -xzf geant4-v11.0.0.tar.gz
mkdir geant4-v11.0.0-build
cd geant4-v11.0.0-build

# Configure and build Geant4
cmake -DCMAKE_INSTALL_PREFIX=/usr/local/geant4 \
  -DGEANT4_USE_OPENGL_X11=ON \
  -DGEANT4_USE_QT=ON \
  -DGEANT4_BUILD_MULTITHREADED=ON \
  -DGEANT4_INSTALL_DATA=ON \
  -DGEANT4_USE_SYSTEM_EXPAT=OFF \
  ../geant4-v11.0.0

make -j$(nproc)
sudo make install

# Set up environment variables
echo 'source /usr/local/geant4/bin/geant4.sh' >> ~/.bashrc
source ~/.bashrc
```

### Installing the 5CB Liquid Crystal Detector Simulation

```bash
# Clone the repository (if using git) or extract from archive
git clone https://github.com/yourusername/LCDetector.git
# Or extract from archive: tar -xzf LCDetector.tar.gz

# Navigate to project directory
cd LCDetector

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)
```

## Detector Details

### Physical Properties
- **Liquid Crystal Material**: 5CB (4-Cyano-4'-pentylbiphenyl) with molecular formula C18H19N
- **Active Volume**: 15mm × 25mm × 100μm (37.5 mm³)
- **Electrodes**: ITO glass (configured to be minimally interacting with beam particles)
- **Bias Voltage**: Variable, default 300V (creates 3V/μm electric field)
- **Electrometer**: Modeled with realistic parameters:
  - 1GΩ input resistance
  - 10pF input capacitance
  - RC time constant circuit modeling
  - Current sampling at 1MHz

### Physics Processes
- **Ionization**: Creates electron-ion pairs in the liquid crystal
- **Charge Transport**: Models electron and ion drift in the electric field
- **Charge Collection**: Simulates collection at electrodes with realistic timing
- **Current Generation**: Calculates induced currents with detailed time profile
- **Field Effects**: Uniform electric field along detector thickness (Y-axis)

## Usage

### Basic Execution

```bash
# Run with default settings (proton beam at 500 MeV)
./LCDetector

# Run with specific particle type and energy
./LCDetector --particle proton --energy 100 MeV

# Run with specific macro file
./LCDetector macros/run_proton.mac
```

### Command-Line Options

```
Usage: ./LCDetector [options] [macro]
Options:
  --particle TYPE    Set particle type (proton, e-, gamma, etc.)
  --energy VALUE     Set particle energy (with unit: 10 MeV, 1 GeV, etc.)
  --help             Show this help message
```

### Available Macro Files

The simulation includes various pre-configured macro files for different types of studies:

#### Basic Run Macros
- **run_proton.mac**: Basic proton beam simulation at different energies
- **run_electron.mac**: Electron beam simulation for beta sources
- **run_gamma.mac**: Gamma ray simulation at energies typical of radioactive sources
- **run_positron.mac**: Positron beam simulation focusing on annihilation detection
- **batch.mac**: High-performance batch mode for large number of events

#### Special Study Macros
- **angular_dependence.mac**: Tests detector response at different incidence angles
- **alpha_particles.mac**: Simulates alpha particles from common radioactive sources
- **background_radiation.mac**: Simulates typical environmental background radiation
- **bias_study.mac**: Investigates effects of varying detector bias voltage
- **neutron_simulation.mac**: Studies neutron interactions at different energies
- **production_run.mac**: High-statistics production run for detailed measurements

#### Visualization and Control
- **vis.mac**: Sets up visualization for detector geometry inspection
- **gun_commands.mac**: Helper for beam direction control

### Runtime Commands

During interactive sessions, you can use these commands:

```
# Set particle type
/LC/beam/particle proton

# Set particle energy
/LC/beam/energy 100 MeV

# Set detector bias voltage
/LC/detector/bias 300 volt

# Enable/disable glass filter (attenuates beam)
/LC/beam/glassFilter true

# Set beam direction (requires gun_commands.mac)
/gun/direction 0 1 0
```

## Output Data

### File Formats
- **ROOT files**: Primary output containing all histograms and ntuples
- **CSV files**: Simple text format for easier processing
- **Text reports**: Summary statistics and configuration details

### Data Structure
The output includes:
1. **Energy deposition**: Total energy deposited in the liquid crystal
2. **Charge collection**: Number of electron-ion pairs and total charge
3. **Current measurements**: Average, peak, and time-profile of electrometer current
4. **Spatial distribution**: Location of interaction events in the detector
5. **Time profiles**: Current vs. time measurements for each event

### Example Analysis
```cpp
// ROOT macro to plot current vs. time
void plotCurrent() {
  TFile f("LC_proton_100MeV.root");
  TH2D* h = (TH2D*)f.Get("CurrentTime");
  h->SetTitle("Electrometer Current vs Time");
  h->GetXaxis()->SetTitle("Time [ns]");
  h->GetYaxis()->SetTitle("Current [pA]");
  h->Draw("COLZ");
}
```

## Troubleshooting

### Common Issues

1. **CMake Error: The current CMakeCache.txt directory is different...**
   - Solution: Clean up and start with a fresh build directory
   ```bash
   rm -f ~/.local/share/Trash/files/CMakeCache.txt
   rm -rf build/
   mkdir build
   cd build
   cmake ..
   make
   ```

2. **Compilation Error: G4UniformElectricField has no member named SetFieldValue**
   - Solution: This is a known issue with the electric field update mechanism
   - Update the `SetBias` method in `LCDetectorConstruction.cc` to recreate the field objects

3. **Visualization Issues**
   - If OpenGL visualization fails, try running with:
   ```bash
   ./LCDetector macros/vis.mac -nogl
   ```

4. **Memory Errors**
   - For large runs, increase available memory or reduce the number of threads:
   ```bash
   # In main.cc, change:
   G4int nThreads = 12;
   # to a lower number:
   G4int nThreads = 4;
   ```

## Advanced Configuration

### Beam Parameters
The simulation supports a wide range of particle types including but not limited to:
- Protons
- Electrons
- Positrons
- Gamma rays
- Alpha particles
- Neutrons
- Muons
- Various ions

Energy ranges from eV to GeV can be simulated depending on the physics processes of interest.

### Detector Modifications
For advanced users, detector parameters can be modified in `LCDetectorConstruction.cc`:
- Liquid crystal dimensions
- Electrode material and thickness
- Electric field configuration
- Material composition

## Support
If you encounter any problems, you can contact: andreis.edu.use@protonmail.com
